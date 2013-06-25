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

class DiskDatastoreIterator {
 public:
  typedef int size_type;
  typedef DiskDatastoreIterator self_type;
  typedef proto::Value value_type;
  typedef proto::Value& reference;
  typedef proto::Value* pointer;
  typedef std::forward_iterator_tag iterator_category;
  typedef int difference_type;

  typedef boost::function<bool(const reference)> callback;

  DiskDatastoreIterator(callback include_callback)
    : node_(NULL),
      include_callback_(include_callback) {}
  DiskDatastoreIterator() : node_(NULL) {
    ++(*this);
  }

  void AddCallback(callback include_callback) {
    include_callback_ = include_callback;
  }

  void AddStream(proto::ValueStream *stream) {
    streams_.push_back(stream);
    stream_pos_.push_back(0);
  }

  DiskDatastoreIterator end() {
    return DiskDatastoreIterator();
  }

  static bool IncludeBetweenTimestamps(const Timestamp &start, const Timestamp &end, const reference node) {
    return (node.timestamp() >= start.ms() && node.timestamp() < end.ms());
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

class DiskDatastore : private noncopyable {
 public:
  typedef unordered_map<string, proto::ValueStream> MapType;

  explicit DiskDatastore(const string &basedir);
  ~DiskDatastore();

  DiskDatastoreIterator find(const Variable &search, const Timestamp &start, const Timestamp &end);
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
