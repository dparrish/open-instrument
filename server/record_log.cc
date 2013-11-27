/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <string>
#include <libgen.h>
#include "lib/common.h"
#include "lib/openinstrument.pb.h"
#include "lib/protobuf.h"
#include "lib/string.h"
#include "lib/timer.h"
#include "lib/trie.h"
#include "lib/variable.h"
#include "server/record_log.h"

DEFINE_int32(recordlog_max_log_size, 100, "Size of record log file in mb");

namespace openinstrument {

RecordLog::RecordLog(const string &basedir)
  : basedir_(basedir),
    shutdown_(false),
    admin_thread_(new thread(bind(&RecordLog::AdminThread, this))),
    replay_reader_(NULL) {
}

RecordLog::~RecordLog() {
  Shutdown();
  if (admin_thread_.get())
    admin_thread_->join();
}

void RecordLog::Add(const proto::ValueStream &stream) {
  MutexLock locker(mutex_);
  log_.push_back(proto::ValueStream());
  log_.back().CopyFrom(stream);
}

// Helper method to add a single Value.
// This creates a temporary ValueStream and adds it to the log.
void RecordLog::Add(const Variable &variable, const proto::Value &value) {
  proto::ValueStream newstream;
  variable.ToProtobuf(newstream.mutable_variable());
  proto::Value *val = newstream.add_value();
  val->CopyFrom(value);
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
    for (proto::ValueStream &stream : log_) {
      if (!writer.Write(stream)) {
        LOG(ERROR) << "Couldn't write stream to recordlog";
        break;
      }
      done_streams++;
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

void RecordLog::RotateRecordLog() {
  FileStat stat(filename());
  if (!stat.exists())
    return;
  if (stat.size() >= (FLAGS_recordlog_max_log_size * 1024 * 1024)) {
    MutexLock locker(mutex_);
    string newfilename = StringPrintf("%s.%s", filename().c_str(), Timestamp().GmTime("%Y-%m-%d-%H-%M-%S.%.").c_str());
    if (::rename(filename().c_str(), newfilename.c_str()) < 0) {
      LOG(WARNING) << "Error renaming " << filename() << " to " << newfilename << ": " << strerror(errno);
    }
    // Create a new empty file
    File fh(filename(), "w");
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
    RotateRecordLog();
    ReindexRecordLog();
    sleep(sleep_time);
    if (shutdown_)
      break;
  }
}

bool RecordLog::ReplayLog(proto::ValueStream *stream) {
  MutexLock locker(mutex_);
  if (replayfiles_.empty()) {
    // First ReplayLog call, build a list of all the files that can be replayed
    replayfiles_ = Glob(filename() + ".*");
    replayfiles_.push_back(filename());
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

bool RecordLog::ReindexRecordLogFile(const string &input, MapType *log_data) const {
  LOG(INFO) << "Reindexing file " << input;
  ProtoStreamReader reader(input);
  proto::ValueStream stream;
  while (reader.Next(&stream)) {
    Variable variable(stream.variable());
    if (variable.ToString().empty())
      continue;
    MapType::iterator it = log_data->find(variable.ToString());
    if (it == log_data->end()) {
      proto::ValueStream &newstream = (*log_data)[variable.ToString()] = proto::ValueStream();
      variable.ToProtobuf(newstream.mutable_variable());
      it = log_data->find(variable.ToString());
    }
    CHECK(it != log_data->end());
    for (auto &oldvalue : stream.value()) {
      proto::Value *lastvalue = it->second.mutable_value(it->second.value_size() - 1);
      if (it->second.value_size() &&
          ((lastvalue->has_string_value() && lastvalue->string_value() == oldvalue.string_value()) ||
          (lastvalue->has_double_value() && lastvalue->double_value() == oldvalue.double_value()))) {
        // Candidate for RLE
        lastvalue->set_end_timestamp(oldvalue.timestamp());
        continue;
      }
      proto::Value *value = it->second.add_value();
      value->CopyFrom(oldvalue);
    }
  }
  return true;
}

bool RecordLog::ReindexRecordLog() {
  bool success = true;
  vector<string> files = Glob(filename() + ".*");
  for (string &filename : files) {
    MapType log_data;
    if (!ReindexRecordLogFile(filename, &log_data) || !WriteIndexedFile(log_data, filename)) {
      string newfile = StringPrintf("failed-%s", filename.c_str());
      LOG(ERROR) << "Failed to reindex record log file, renamed it to " << newfile;
      rename(filename.c_str(), newfile.c_str());
      success = false;
    }
  }
  return success;
}

bool RecordLog::WriteIndexedFile(RecordLog::MapType &log_data, const string &filename) {
  uint64_t output_streams = 0, output_values = 0;
  uint64_t newest_timestamp = 0;
  proto::StoreFileHeader header;
  // Build the output header
  for (MapType::iterator i = log_data.begin(); i != log_data.end(); ++i) {
    proto::StreamVariable *var = header.add_variable();
    var->CopyFrom(i->second.variable());

    proto::StoreFileHeaderIndex *index = header.add_index();
    index->mutable_variable()->CopyFrom(i->second.variable());
    index->set_offset(0);

    output_values += i->second.value_size();
    output_streams++;
    if (i->second.value_size()) {
      const proto::Value &first_value = i->second.value(0);
      if (!header.has_start_timestamp() || first_value.timestamp() < header.start_timestamp())
        header.set_start_timestamp(first_value.timestamp());
      const proto::Value &last_value = i->second.value(i->second.value_size() - 1);
      if (last_value.timestamp() > header.end_timestamp())
        header.set_end_timestamp(last_value.timestamp());
      if (last_value.end_timestamp() > header.end_timestamp())
        header.set_end_timestamp(last_value.end_timestamp());
    }
  }

  string outfile = StringPrintf("%s.new", filename.c_str());
  FileStat stat(outfile);
  if (stat.exists()) {
    LOG(ERROR) << "Output file " << outfile << " already exists";
    return false;
  }

  // Write all ValueStreams
  ProtoStreamWriter writer(outfile);
  if (!writer.fh()) {
    LOG(ERROR) << "Can't write reindexed record log to " << outfile;
    return false;
  }
  if (!writer.Write(header)) {
    LOG(ERROR) << "Can't write reindexed record log header to " << outfile;
    return false;
  }

  int j = 0;
  for (RecordLog::MapType::iterator i = log_data.begin(); i != log_data.end(); ++i) {
    proto::StoreFileHeaderIndex *index = header.mutable_index(j++);
    index->set_offset(writer.fh()->Tell());
    if (!writer.Write(i->second)) {
      LOG(ERROR) << "Can't write reindexed record log row to " << outfile;
      return false;
    }
    for (auto value : i->second.value()) {
      if (value.timestamp() > newest_timestamp)
        newest_timestamp = value.timestamp();
    }
  }

  // Write the header at the beginning of the file
  if (writer.fh()->SeekAbs(0) != 0) {
    LOG(ERROR) << "Can't seek to the beginning of " << outfile << " to re-write header: " << strerror(errno);
    return false;
  }
  if (!writer.Write(header)) {
    LOG(ERROR) << "Can't re-write reindexed record log header to " << outfile;
    return false;
  }

  if (unlink(filename.c_str())) {
    LOG(ERROR) << "Can't remove original file " << filename << ": " << strerror(errno);
    return false;
  }
  char *dir = dirname(strdup(outfile.c_str()));
  string finalfile = StringPrintf("%s/datastore.%lu.bin", dir, newest_timestamp);
  free(dir);
  if (::rename(outfile.c_str(), finalfile.c_str()) < 0) {
    LOG(ERROR) << "Can't rename " << outfile << " to " << finalfile << ": " << strerror(errno);
    return false;
  }
  LOG(INFO) << "Created indexed file " << finalfile << " containing " << output_streams << " streams and "
            << output_values << " values, between " << Timestamp(header.start_timestamp()).GmTime() << " and "
            << Timestamp(header.end_timestamp()).GmTime();
  return true;
}

}  // namespace openinstrument
