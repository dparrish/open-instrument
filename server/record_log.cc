/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <string>
#include "lib/common.h"
#include "lib/openinstrument.pb.h"
#include "lib/protobuf.h"
#include "lib/string.h"
#include "lib/timer.h"
#include "lib/trie.h"
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
  variable.CopyTo(newstream.mutable_variable());
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
    LOG(INFO) << "recordlog " << filename() << " should be rotated";
    string newfilename = StringPrintf("%s.%s", filename().c_str(), Timestamp().GmTime("%Y-%m-%d-%H-%M-%S.%.").c_str());
    LOG(INFO) << "  rename to " << newfilename;
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

void RecordLog::ReindexRecordLog() {
  vector<string> files = Glob(filename() + ".*");
  typedef Trie<proto::ValueStream> MapType;
  MapType log_data;
  BOOST_FOREACH(string &filename, files) {
    proto::StoreFileHeader header;
    uint64_t input_streams = 0, input_values = 0;
    uint64_t output_streams = 0;
    LOG(INFO) << "Reindexing file " << filename;
    ProtoStreamReader reader(filename);
    proto::ValueStream stream;
    while (reader.Next(&stream)) {
      input_streams++;

      Variable search;
      if (stream.has_old_variable()) {
        // Old style variable name, convert to new
        search.FromString(stream.old_variable());
        if (search.GetLabel("datatype") == "gauge") {
          search.mutable_proto()->set_value_type(proto::Variable::GAUGE);
          search.RemoveLabel("datatype");
        } else if (search.GetLabel("datatype") == "counter") {
          search.mutable_proto()->set_value_type(proto::Variable::COUNTER);
          search.RemoveLabel("datatype");
        }
      } else {
        search.CopyFrom(stream.variable());
      }
      MapType::iterator it = log_data.find(search.ToString());
      if (it == log_data.end()) {
        log_data.insert(search.ToString(), proto::ValueStream());
        it = log_data.find(search.ToString());
        it->mutable_variable()->CopyFrom(stream.variable());
        output_streams++;
      }
      CHECK(it != log_data.end());
      for (int i = 0; i < stream.value_size(); i++) {
        proto::Value *value = it->add_value();
        value->set_timestamp(stream.value(i).timestamp());
        value->set_value(stream.value(i).value());
        if (!header.has_start_timestamp() || value->timestamp() < header.start_timestamp())
          header.set_start_timestamp(value->timestamp());
        if (value->timestamp() > header.end_timestamp())
          header.set_end_timestamp(value->timestamp());
        input_values++;
      }
    }

    // Build the output header
    for (MapType::iterator i = log_data.begin(); i != log_data.end(); ++i) {
      proto::Variable *var = header.add_variable();
      var->CopyFrom(Variable(i.key()).proto());
    }
    string outfile = StringPrintf("%s/datastore.%llu.bin", basedir_.c_str(), header.end_timestamp());

    // Write the header and all ValueStreams
    ProtoStreamWriter writer(outfile);
    writer.Write(header);
    for (MapType::iterator i = log_data.begin(); i != log_data.end(); ++i) {
      writer.Write(*i);
    }
    LOG(INFO) << "Created indexed file " << outfile << " containing " << output_streams << " streams and "
              << input_values << " values, between " << Timestamp(header.start_timestamp()).GmTime() << " and "
              << Timestamp(header.end_timestamp()).GmTime();
    ::unlink(filename.c_str());
  }
}

}  // namespace openinstrument
