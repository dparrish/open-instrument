/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <vector>
#include <string>
#include "lib/common.h"
#include "lib/file.h"
#include "lib/protobuf.h"
#include "lib/string.h"
#include "lib/timer.h"
#include "server/disk_datastore.h"
#include "server/indexed_store_file.h"
#include "server/record_log.h"

namespace openinstrument {

DiskDatastore::DiskDatastore(const string &basedir)
  : basedir_(basedir),
    record_log_(basedir_) {
  ReplayRecordLog();
}

DiskDatastore::~DiskDatastore() {
  live_data_.clear();
}

Datastore::iterator DiskDatastore::find(const Variable &search, const Timestamp &start, const Timestamp &end) {
  Datastore::iterator it(bind(&Datastore::iterator::IncludeBetweenTimestamps, start, end, _1));
  for (auto &variable : FindVariables(search))
    it.AddStream(&GetValueStream(variable));
  return ++it;
}

void DiskDatastore::GetRange(const Variable &variable, const Timestamp &start, const Timestamp &end,
                             proto::ValueStream *outstream) {
  try {
    proto::ValueStream &instream = GetValueStream(variable);
    if (!instream.value_size())
      return;
    outstream->mutable_variable()->CopyFrom(instream.variable());
    for (auto &value : instream.value()) {
      if (value.timestamp() >= static_cast<uint64_t>(start.ms()) &&
          ((end.ms() && value.timestamp() < static_cast<uint64_t>(end.ms())) || !end.ms())) {
        proto::Value *val = outstream->add_value();
        val->CopyFrom(value);
      }
    }
  } catch (exception) {
    return;
  }
}

set<Variable> DiskDatastore::FindVariables(const Variable &variable) {
  set<Variable> vars;
  for (MapType::iterator i = live_data_.begin(); i != live_data_.end(); ++i) {
    if (Variable(i->first).Matches(variable))
      vars.insert(Variable(i->first));
  }
  return vars;
}

proto::ValueStream &DiskDatastore::GetOrCreateVariable(const Variable &variable) {
  try {
    proto::ValueStream &stream = GetValueStream(variable);
    return stream;
  } catch (exception) {
    return CreateVariable(variable);
  }
}

proto::ValueStream &DiskDatastore::GetValueStream(const Variable &variable) {
  MapType::iterator it = live_data_.find(variable.ToString());
  if (it == live_data_.end())
    throw out_of_range("Variable not found");
  return it->second;
}

proto::ValueStream &DiskDatastore::CreateVariable(const Variable &variable) {
  proto::ValueStream stream;
  variable.ToValueStream(&stream);
  live_data_[variable.ToString()] = stream;
  return live_data_[variable.ToString()];
}

proto::Value *DiskDatastore::RecordNoLog(const Variable &variable, Timestamp timestamp, const proto::Value &value) {
  proto::ValueStream &stream = GetOrCreateVariable(variable);
  proto::Value *val = stream.add_value();
  val->CopyFrom(value);
  val->set_timestamp(timestamp.ms());
  return val;
}

void DiskDatastore::ReplayRecordLog() {
  try {
    VLOG(1) << "Replaying record log";
    proto::ValueStream stream;
    uint64_t num_points = 0, num_streams = 0;
    while (record_log_.ReplayLog(&stream)) {
      for (auto &value : stream.value()) {
        RecordNoLog(Variable(stream.variable()), value.timestamp(), value);
        num_points++;
      }
      num_streams++;
    }
    LOG(INFO) << "Replayed record log, got " << num_points << " points" << " in " << num_streams << " streams";
  } catch (exception &e) {
    LOG(WARNING) << "Couldn't replay record log: " << e.what();
  }
}

Datastore::iterator::self_type Datastore::iterator::operator++() {
  uint64_t min_timestamp_ = 0;
  int min_timestamp_stream_ = 0;
  if (streams_.size() == 0) {
    // End of the list, no streams to process
    LOG(ERROR) << "No streams";
    this->node_ = NULL;
    return *this;
  }
  for (size_t i = 0; i < streams_.size(); ++i) {
    proto::ValueStream *stream = streams_[i];
    if (stream_pos_[i] >= stream->value_size()) {
      // Reached the end of a stream
      continue;
    }
    while (true) {
      if (stream_pos_[i] >= stream->value_size()) {
        // Reached the end of a stream
        break;
      }
      proto::Value *node = stream->mutable_value(stream_pos_[i]);
      if (!include_callback_(*node)) {
        stream_pos_[i]++;
        continue;
      }
      // There's a valid node.
      if (!min_timestamp_ || min_timestamp_ > node->timestamp()) {
        // This is the oldest node so far, remember it for later
        min_timestamp_ = node->timestamp();
        min_timestamp_stream_ = i;
      }
      break;
    }
  }
  if (min_timestamp_) {
    // An item was found.
    this->node_ = streams_[min_timestamp_stream_]->mutable_value(stream_pos_[min_timestamp_stream_]);
    this->node_->mutable_variable()->CopyFrom(streams_[min_timestamp_stream_]->variable());
    // Increment the pointer so this same item isn't returned again
    stream_pos_[min_timestamp_stream_]++;
    return *this;
  }
  this->node_ = NULL;
  return *this;
}

}  // namespace
