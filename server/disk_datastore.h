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
#include "lib/variable.h"
#include "server/indexed_store_file.h"
#include "server/record_log.h"

namespace openinstrument {

class DiskDatastore : private noncopyable {
 public:
  typedef unordered_map<string, proto::ValueStream> MapType;

  explicit DiskDatastore(const string &basedir);
  ~DiskDatastore();

  void GetRange(const Variable &variable, const Timestamp &start, const Timestamp &end, proto::ValueStream *outstream);
  set<Variable> FindVariables(const Variable &variable);

  inline void Record(const Variable &variable, Timestamp timestamp, const proto::Value &value) {
    proto::Value *val = RecordNoLog(variable, timestamp, value);
    if (val)
      record_log_.Add(variable, *val);
  }

  inline void Record(const Variable &variable, const proto::Value &value) {
    Record(variable, Timestamp::Now(), value);
  }

  class IndexedFile : private noncopyable {
   public:
    IndexedFile(const string &filename);
    vector<proto::ValueStream> GetVariable(const Variable &variable);
    bool GetVariable(const Variable &variable, vector<proto::ValueStream> *results);

    string filename;
    ProtoStreamReader reader;
    MapType log_data;
    proto::StoreFileHeader header;
  };

  IndexedFile *OpenIndexedFile(const string &filename) {
    return new IndexedFile(filename);
  }

 private:
  proto::ValueStream &GetOrCreateVariable(const Variable &variable);
  proto::ValueStream &GetVariable(const Variable &variable);
  proto::ValueStream &CreateVariable(const Variable &variable);
  proto::Value *RecordNoLog(const Variable &variable, Timestamp timestamp, const proto::Value &value);
  void ReplayRecordLog();

  string basedir_;
  MapType live_data_;
  RecordLog record_log_;
};

}  // namespace

#endif  // _OPENINSTRUMENT_LIB_DISK_DATASTORE_H
