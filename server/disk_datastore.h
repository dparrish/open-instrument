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

class DiskDatastoreIterator : public boost::iterator_facade<
                          DiskDatastoreIterator,
                          proto::Value,
                          boost::forward_traversal_tag> {
                          // http://www.boost.org/doc/libs/1_53_0/libs/iterator/doc/new-iter-concepts.html#forward-traversal-iterators-lib-forward-traversal-iterators
                          // boost:bidirectional_traversal_tag
                          // boost:random_access_traversal_tag
 public:
  DiskDatastoreIterator(const Timestamp &start, const Timestamp &end)
    : node_(NULL),
      start_(start),
      end_(end) {
  }

  void AddStream(proto::ValueStream *stream) {
    streams_.push_back(stream);
    stream_pos_.push_back(0);
  }

  DiskDatastoreIterator end() const {
    return DiskDatastoreIterator(0, 0);
  }

 private:
  friend class boost::iterator_core_access;

  virtual void increment() {
    uint64_t min_timestamp_ = 0;
    int min_timestamp_stream_ = 0;
    if (!streams_.size()) {
      // End of the list, no streams to process
      this->node_ = NULL;
      return;
    }
    for (size_t i = 0; i < streams_.size(); ++i) {
      proto::ValueStream *stream = streams_[i];
      if (stream_pos_[i] >= stream->value_size()) {
        // Reached the end of a stream
        continue;
      }
      proto::Value *value = stream->mutable_value(stream_pos_[i]);
      while (value->timestamp() < start_.ms() && stream_pos_[i] < stream->value_size()) {
        // Look for a value that is after the start timestamp
        stream_pos_[i]++;
        //LOG(INFO) << "Discarding timestamp " << value->timestamp() << " which is before start time " << start_.ms();
        value = stream->mutable_value(stream_pos_[i]);
      }
      if (stream_pos_[i] >= stream->value_size()) {
        // Reached the end of a stream
        continue;
      }
      if (value->timestamp() > end_.ms()) {
        // Value is after end timestamp
        //LOG(INFO) << "Discarding timestamp " << value->timestamp() << " which is after end time " << end_.ms();
        continue;
      }
      if (!min_timestamp_ || value->timestamp() <= min_timestamp_) {
        // Found a valid value, remember it for later
        min_timestamp_ = value->timestamp();
        min_timestamp_stream_ = i;
      }
    }
    if (min_timestamp_) {
      // An item was found.
      this->node_ = streams_[min_timestamp_stream_]->mutable_value(stream_pos_[min_timestamp_stream_]);
      this->node_->mutable_variable()->CopyFrom(streams_[min_timestamp_stream_]->variable());
      // Increment the pointer so this same item isn't returned again
      stream_pos_[min_timestamp_stream_]++;
      return;
    }
    this->node_ = NULL;
  }

  bool equal(DiskDatastoreIterator const &other) const {
    return node_ == other.node_;
  }

  proto::Value& dereference() const {
    return *node_;
  }

  proto::Value *node_;
  vector<proto::ValueStream *> streams_;
  vector<int> stream_pos_;
  const Timestamp &start_;
  const Timestamp &end_;
};

class DiskDatastore : private noncopyable {
 public:
  typedef unordered_map<string, proto::ValueStream> MapType;

  explicit DiskDatastore(const string &basedir);
  ~DiskDatastore();

  void GetRange(const Variable &variable, const Timestamp &start, const Timestamp &end, proto::ValueStream *outstream);
  DiskDatastoreIterator GetRange(const Variable &variable, const Timestamp &start, const Timestamp &end);
  set<Variable> FindVariables(const Variable &variable);

  inline void Record(const Variable &variable, Timestamp timestamp, const proto::Value &value) {
    proto::Value *val = RecordNoLog(variable, timestamp, value);
    if (val)
      record_log_.Add(variable, *val);
  }

  inline void Record(const Variable &variable, const proto::Value &value) {
    Record(variable, Timestamp::Now(), value);
  }

  proto::ValueStream &GetVariable(const Variable &variable);

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
