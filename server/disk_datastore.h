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
    reference operator*() { return *node_; }
    pointer operator->() { return node_; }
    bool operator==(const self_type &rhs) { return node_ == rhs.node_; }
    bool operator!=(const self_type &rhs) { return node_ != rhs.node_; }

   private:
    pointer node_;
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
  virtual proto::ValueStream &GetValueStream(const Variable &variable) = 0;

  // Iterate over variable values matching a search critera and start/end timestamps.
  virtual iterator find(const Variable &search, const Timestamp &start, const Timestamp &end) = 0;
};

class BasicDatastore : public Datastore {
 public:
  BasicDatastore() {}
  ~BasicDatastore() {}

  virtual proto::ValueStream &GetValueStream(const Variable &variable) {
    for (proto::ValueStream *stream : streams_) {
      if (Variable(stream->variable()) == variable)
        return *stream;
    }
    auto stream = new proto::ValueStream();
    variable.ToProtobuf(stream->mutable_variable());
    streams_.push_back(stream);
    return *stream;
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
    for (auto stream : streams_) {
      if (Variable(stream->variable()).Matches(search))
        it.AddStream(stream);
    }
    return ++it;
  }

 private:
  vector<proto::ValueStream *> streams_;
};

class DiskDatastore : public Datastore {
 public:
  typedef unordered_map<string, proto::ValueStream> MapType;

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

  proto::ValueStream &GetValueStream(const Variable &variable);

 private:
  proto::ValueStream &GetOrCreateVariable(const Variable &variable);
  proto::ValueStream &CreateVariable(const Variable &variable);
  proto::Value *RecordNoLog(const Variable &variable, Timestamp timestamp, const proto::Value &value);
  void ReplayRecordLog();

  string basedir_;
  MapType live_data_;
  RecordLog record_log_;
};

}  // namespace

#endif  // _OPENINSTRUMENT_LIB_DISK_DATASTORE_H
