/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <string>
#include <glob.h>
#include "lib/common.h"
#include "lib/openinstrument.pb.h"
#include "lib/protobuf.h"
#include "lib/string.h"
#include "lib/timer.h"
#include "server/record_log.h"

DEFINE_int32(recordlog_max_log_size, 10, "Size of record log file in mb");

namespace openinstrument {

uint64_t RecordLog::Add(proto::ValueStream &stream) {
  MutexLock locker(mutex_);
  if (!streams() || log_.back().variable() != stream.variable()) {
    proto::ValueStream newstream;
    newstream.set_variable(stream.variable());
    log_.push_back(newstream);
  }
  for (int i = 0; i < stream.value_size(); i++) {
    proto::Value *newval = log_.back().add_value();
    const proto::Value &oldval = stream.value(i);
    newval->set_timestamp(oldval.timestamp());
    newval->set_value(oldval.value());
    size_++;
  }
  return size_;
}

// Helper method to add a single Value.
// This creates a temporary ValueStream and adds it to the log.
uint64_t RecordLog::Add(const string &variable, const proto::Value &value) {
  proto::ValueStream newstream;
  newstream.set_variable(variable);
  proto::Value *val = newstream.add_value();
  val->set_timestamp(value.timestamp());
  val->set_value(value.value());
  return Add(newstream);
}

// Flush as many streams as possible to the record log.
// No attempt is made to re-order or compress the streams so this is a very inefficient (and fast) way of recording
// the data.
// Returns false if there are more streams left to flush, returns true if the whole log has been flushed to disk.
bool RecordLog::Flush() {
  if (!streams())
    return true;
  MutexLock locker(mutex_);
  try {
    ProtoStreamWriter writer(filename());
    uint64_t done_streams = 0;
    BOOST_FOREACH(proto::ValueStream &stream, log_) {
      if (!writer.Write(stream)) {
        LOG(ERROR) << "Couldn't write stream to recordlog";
        break;
      }
      done_streams++;
      size_ -= stream.value_size();
    }
    if (done_streams) {
      log_.erase(log_.begin(), log_.begin() + done_streams);
    }
    VLOG(1) << "Flushed " << done_streams << " to recordlog, there are " << streams() << " streams left";
  } catch (exception &e) {
    LOG(ERROR) << "Can't write to recordlog: " << e.what();
    return false;
  }
  return true;
}

void RecordLog::RotateFlushLogs() {
  FileStat stat(filename());
  if (!stat.exists())
    return;
  if (stat.size() >= (FLAGS_recordlog_max_log_size * 1024 * 1024)) {
    MutexLock locker(mutex_);
    LOG(INFO) << "recordlog " << filename() << " should be rotated";
    string newfilename = StringPrintf("%s.%s", filename().c_str(), Timestamp().GmTime("%Y-%m-%d-%H-%M-%S.%.").c_str());
    LOG(INFO) << "  rename to " << newfilename;
    if (::rename(filename().c_str(), newfilename.c_str()) < 0) {
      LOG(WARNING) << "Error renaming " << filename() << " to " << newfilename << ": " << strerror(errno);
    }
    // Create a new empty file
    LocalFile fh(filename(), "w");
  }
}

void RecordLog::AdminThread() {
  const int sleep_time = 2;
  while (true) {
    if (streams()) {
      // Flush out any ValueStreams that are in ram and have not yet been written to disk.
      if (!Flush()) {
        LOG(INFO) << "RecordLog retrying in " << sleep_time << " seconds";
      }
    }
    RotateFlushLogs();
    sleep(sleep_time);
    if (shutdown_)
      break;
  }
}

bool RecordLog::ReplayLog(proto::ValueStream *stream) {
  MutexLock locker(mutex_);
  if (replayfiles_.empty()) {
    // First ReplayLog call, build a list of all the files that can be replayed
    string pattern = filename() + ".*";
    glob_t pglob;
    if (::glob(pattern.c_str(), 0, NULL, &pglob) == 0) {
      replayfiles_.reserve(pglob.gl_pathc + 1);
      for (size_t i = 0; i < pglob.gl_pathc; i++)
        replayfiles_.push_back(pglob.gl_pathv[i]);
    }
    replayfiles_.push_back(filename());
    ::globfree(&pglob);
  }
  while (replayfiles_.size()) {
    if (!replay_reader_.get()) {
      // No file is currently in progress, open one
      if (replayfiles_.empty())
        return NULL;
      string filename = replayfiles_.front();
      replay_reader_.reset(new ProtoStreamReader(filename));
      VLOG(1) << "Replaying " << filename;
    }
    if (!replay_reader_->Next(stream)) {
      replay_reader_.reset(NULL);
      replayfiles_.erase(replayfiles_.begin());
      continue;
    }
    return true;
  }
  return false;
}

}  // namespace openinstrument
