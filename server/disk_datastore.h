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
#include "lib/common.h"
#include "lib/file.h"
#include "lib/protobuf.h"
#include "lib/string.h"
#include "lib/timer.h"
#include "server/datastore.h"
#include "server/record_log.h"

namespace openinstrument {

class DiskDatastore : private noncopyable {
 public:
  typedef unordered_map<string, proto::ValueStream> MapType;

  explicit DiskDatastore(const string &basedir);
  ~DiskDatastore();

  void GetRange(const Variable &variable, const Timestamp &start, const Timestamp &end, proto::ValueStream *outstream);
  set<Variable> FindVariables(const Variable &variable);

  inline bool CompareMessage(const proto::ValueStream &a, const proto::ValueStream &b) {
    return Variable(a.variable()) == Variable(b.variable());
  }

  inline bool CompareMessage(const proto::Value &a, const proto::Value &b) {
    return a.timestamp() == b.timestamp() && a.value() == b.value();
  }

  inline void Record(const Variable &variable, Timestamp timestamp, double value) {
    proto::Value *val = RecordNoLog(variable, timestamp, value);
    if (val)
      record_log_.Add(variable, *val);
  }

  inline void Record(const string &variable, double value) {
    Record(variable, Timestamp::Now(), value);
  }

 private:
  proto::ValueStream &GetOrCreateVariable(const Variable &variable);
  proto::ValueStream &GetVariable(const Variable &variable);
  proto::ValueStream &CreateVariable(const Variable &variable);
  proto::Value *RecordNoLog(const Variable &variable, Timestamp timestamp, double value);
  void ReplayRecordLog();

  string basedir_;
  MapType live_data_;
  RecordLog record_log_;
};

}  // namespace

#endif  // _OPENINSTRUMENT_LIB_DISK_DATASTORE_H
