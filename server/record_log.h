/*
 * Very simple log of ValueStreams.
 *
 * This is used to keep a running log of the last entries added, and is not ordered or even efficient.
 * The log can be replayed at startup to load all items that have been recorded by not yet indexed and stored in a more
 * permanent location.
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef OPENINSTRUMENT_SERVER_RECORD_LOG_H_
#define OPENINSTRUMENT_SERVER_RECORD_LOG_H_

#include <deque>
#include <string>
#include <vector>
#include "lib/common.h"
#include "lib/openinstrument.pb.h"
#include "lib/protobuf.h"
#include "lib/string.h"
#include "lib/trie.h"
#include "lib/variable.h"

namespace openinstrument {

class RecordLog : private noncopyable {
 public:
  typedef proto::ValueStream & iterator_type;
  typedef const proto::ValueStream & const_iterator_type;
  typedef deque<proto::ValueStream>::iterator iterator;
  typedef deque<proto::ValueStream>::const_iterator const_iterator;

  // Create a RecordLog stored at <basedir>/recordlog and a thread that will regularly flush items in memory to disk.
  // This thread will remain active until Shutdown() is called or this object is destroyed.
  explicit RecordLog(const string &basedir);
  ~RecordLog();

  // Tell the admin thread to shut down.
  // It could be up to 5 minutes until the thread actually shuts down.
  inline void Shutdown() {
    LOG(INFO) << "Shutting down RecordLog thread";
    shutdown_ = true;
  }

  // Replay all record logs in order.
  // Each call to ReplayLog will return a ValueStream pointer containing one or more values.
  // When there are no more logs to replay, NULL will be returned.
  bool ReplayLog(proto::ValueStream *stream);

  // Add a single log file to the replay files.
  // This file will have no impact on recording, just playback.
  void AddLogFile(const string &filename) {
    replayfiles_.push_back(filename);
  }

  // Add a ValueStream to the log.
  // At some point in the future this will be flushed to disk, but a successful return from Add() does not guarantee
  // that it is written to disk.
  // Call Flush() until the return is true to force disk output.
  void Add(const proto::ValueStream &stream);

  // Helper method to add a single Value.
  // This creates a temporary ValueStream and adds it to the log.
  void Add(const Variable &variable, const proto::Value &value);

  // Flush as many streams as possible to the record log.
  // No attempt is made to re-order or compress the streams so this is a very inefficient (and fast) way of recording
  // the data.
  // Returns false if there are more streams left to flush, returns true if the whole log has been flushed to disk.
  bool Flush();

  const string filename() const {
    return StringPrintf("%s/recordlog", basedir_.c_str());
  }

  inline uint64_t streams() const {
    return log_.size();
  }

  inline iterator begin() {
    return log_.begin();
  }

  inline iterator end() {
    return log_.end();
  }

  typedef Trie<proto::ValueStream> MapType;
  void ReindexRecordLog();
  void ReindexRecordLogFile(const string &input, MapType *log_data) const;

 private:
  void AdminThread();
  void RotateRecordLog();
  void LoadIndexedFile(const string &filename);

  const string basedir_;
  deque<proto::ValueStream> log_;
  Mutex mutex_;
  bool shutdown_;
  scoped_ptr<thread> admin_thread_;

  // Contains state between repeated ReplayLog calls
  vector<string> replayfiles_;
  scoped_ptr<ProtoStreamReader> replay_reader_;
};

}  // namespace openinstrument

#endif  // OPENINSTRUMENT_SERVER_RECORD_LOG_H_
