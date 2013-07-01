/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_DISK_DATASTORE_H
#define _OPENINSTRUMENT_LIB_DISK_DATASTORE_H

#include <vector>
#include <string>
#include <boost/iterator/iterator_facade.hpp>
#include "lib/common.h"
#include "lib/file.h"
#include "lib/protobuf.h"
#include "lib/string.h"
#include "lib/timer.h"
#include "lib/variable.h"
#include "server/indexed_store_file.h"
#include "server/record_log.h"

namespace openinstrument {

class Datastore : private noncopyable {
 public:
  class iterator {
   public:
    typedef int size_type;
    typedef iterator self_type;
    typedef proto::Value value_type;
    typedef proto::Value& reference;
    typedef proto::Value* pointer;
    typedef std::forward_iterator_tag iterator_category;
    typedef int difference_type;

    typedef boost::function<bool(const reference)> callback;

    iterator(callback include_callback)
      : node_(NULL),
        include_callback_(include_callback) {}
    iterator() : node_(NULL) {}

    void AddCallback(callback include_callback) {
      include_callback_ = include_callback;
    }

    void AddStream(proto::ValueStream *stream) {
      streams_.push_back(stream);
      stream_pos_.push_back(0);
    }

    iterator end() {
      return iterator();
    }

    static bool IncludeBetweenTimestamps(const Timestamp &start, const Timestamp &end, const reference node) {
      if (start.ms() && node.timestamp() < start.ms())
        return false;
      if (end.ms() && node.timestamp() > end.ms())
        return false;
      return true;
    }

    self_type operator++();
    reference operator*() { return nodecopy_; }
    pointer operator->() { return &nodecopy_; }
    bool operator==(const self_type &rhs) { return node_ == rhs.node_; }
    bool operator!=(const self_type &rhs) { return node_ != rhs.node_; }

   private:
    pointer node_;
    value_type nodecopy_;
    vector<proto::ValueStream *> streams_;
    vector<int> stream_pos_;
    callback include_callback_;
  };

  virtual ~Datastore() {}

  // Record the value of a variable at a given time.
  virtual void Record(const Variable &variable, Timestamp timestamp, const proto::Value &value) = 0;

  // Record the value of a variable at the current time.
  virtual void Record(const Variable &variable, const proto::Value &value) {
    Record(variable, Timestamp::Now(), value);
  }

  // List all variables in the store matching the supplied search criteria.
  virtual set<Variable> FindVariables(const Variable &variable) = 0;

  // Return the original ValueStream for a single variable.
  virtual proto::ValueStream *GetValueStream(const Variable &variable) = 0;

  // Iterate over variable values matching a search critera and start/end timestamps.
  virtual iterator find(const Variable &search, const Timestamp &start, const Timestamp &end) = 0;
};

class ValueStream {
 public:
  ValueStream()
    : stream_(new proto::ValueStream()),
      stream_owned_(true) {}
  ValueStream(proto::ValueStream *stream)
    : stream_(stream),
      stream_owned_(false) {}
  ~ValueStream() {
    if (stream_owned_)
      delete stream_;
  }

  proto::ValueStream *release() {
    stream_owned_ = false;
    return stream_;
  }

  void reset(proto::ValueStream *stream) {
    if (stream_owned_)
      delete stream_;
    if (stream) {
      stream_ = stream;
      stream_owned_ = false;
    } else {
      stream_ = new proto::ValueStream();
      stream_owned_ = true;
    }
  }

  proto::ValueStream *operator->() {
    return stream_;
  }

  const proto::ValueStream &operator*() {
    return *stream_;
  }

  Variable variable() const {
    Variable var;
    var.FromProtobuf(stream_->variable());
    return var;
  }

  void set_variable(const Variable &var) {
    var.ToProtobuf(stream_->mutable_variable());
  }

 private:
  proto::ValueStream *stream_;
  bool stream_owned_;
};

class BasicDatastore : public Datastore {
 public:
  BasicDatastore() {}
  ~BasicDatastore() {}

  virtual proto::ValueStream *GetValueStream(const Variable &variable) {
    for (proto::ValueStream *stream : streams_) {
      if (Variable(stream->variable()) == variable)
        return stream;
    }
    auto stream = new proto::ValueStream();
    variable.ToProtobuf(stream->mutable_variable());
    streams_.push_back(stream);
    return stream;
  }

  virtual void Record(const Variable &variable, Timestamp timestamp, const proto::Value &value) {
    proto::ValueStream *stream = NULL;
    for (proto::ValueStream *i : streams_) {
      if (Variable(i->variable()) == variable)
        stream = i;
    }
    if (!stream) {
      stream = new proto::ValueStream();
      variable.ToProtobuf(stream->mutable_variable());
      streams_.push_back(stream);
    }
    proto::Value *new_value = stream->add_value();
    new_value->CopyFrom(value);
    new_value->set_timestamp(timestamp.ms());
  }

  virtual set<Variable> FindVariables(const Variable &variable) {
    set<Variable> vars;
    for (auto i : streams_) {
      if (Variable(i->variable()).Matches(variable))
        vars.insert(Variable(i->variable()));
    }
    return vars;
  }

  virtual iterator find(const Variable &search, const Timestamp &start, const Timestamp &end) {
    iterator it(bind(&iterator::IncludeBetweenTimestamps, start, end, _1));
    int counter = 0;
    for (auto stream : streams_) {
      if (!Variable(stream->variable()).Matches(search))
        continue;
      if (!stream->value_size())
        continue;
      if (stream->value(0).timestamp() > end.ms())
        continue;
      if (stream->value(stream->value_size() - 1).timestamp() < start.ms())
        continue;
      it.AddStream(stream);
      counter++;
    }
    LOG(INFO) << "Iterating over " << counter << " streams";
    return ++it;
  }

 private:
  vector<proto::ValueStream *> streams_;
};

class DiskDatastore : public Datastore {
 public:
  typedef unordered_map<string, proto::ValueStream *> MapType;

  explicit DiskDatastore(const string &basedir);
  ~DiskDatastore();

  Datastore::iterator find(const Variable &search, const Timestamp &start, const Timestamp &end) ;
  void GetRange(const Variable &variable, const Timestamp &start, const Timestamp &end, proto::ValueStream *outstream);
  Datastore::iterator GetRange(const Variable &variable, const Timestamp &start, const Timestamp &end);
  set<Variable> FindVariables(const Variable &variable);

  virtual void Record(const Variable &variable, Timestamp timestamp, const proto::Value &value) {
    proto::Value *val = RecordNoLog(variable, timestamp, value);
    if (val)
      record_log_.Add(variable, *val);
  }

  proto::ValueStream *GetValueStream(const Variable &variable);

 private:
  proto::ValueStream *GetOrCreateVariable(const Variable &variable);
  proto::ValueStream *CreateVariable(const Variable &variable);
  proto::Value *RecordNoLog(const Variable &variable, Timestamp timestamp, const proto::Value &value);
  void ReplayRecordLog();

  string basedir_;
  MapType live_data_;
  RecordLog record_log_;
};

}  // namespace

#endif  // _OPENINSTRUMENT_LIB_DISK_DATASTORE_H
