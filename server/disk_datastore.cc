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
#include "server/datastore.h"
#include "server/disk_datastore.h"
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

void DiskDatastore::GetRange(const string &variable, const Timestamp &start, const Timestamp &end,
                             proto::ValueStream *outstream) {
  try {
    proto::ValueStream &instream = GetVariable(variable);
    if (!instream.value_size())
      return;
    outstream->set_variable(instream.variable());
    for (int i = 0; i < instream.value_size(); i++) {
      const proto::Value &value = instream.value(i);
      if (value.timestamp() >= static_cast<uint64_t>(start.ms()) &&
          ((end.ms() && value.timestamp() < static_cast<uint64_t>(end.ms())) || !end.ms())) {
        proto::Value *val = outstream->add_value();
        val->set_timestamp(value.timestamp());
        val->set_value(value.value());
      }
    }
  } catch (exception) {
    return;
  }
}

set<Variable> DiskDatastore::FindVariables(const string &variable) {
  set<Variable> vars;
  Variable search(variable);
  if (search.variable().empty())
    return vars;
  for (MapType::iterator i = live_data_.begin(); i != live_data_.end(); ++i) {
    if (Variable(i->first).Matches(search))
      vars.insert(Variable(i->first));
  }
  return vars;
}

proto::ValueStream &DiskDatastore::GetOrCreateVariable(const string &variable) {
  try {
    proto::ValueStream &stream = GetVariable(variable);
    return stream;
  } catch (exception) {
    return CreateVariable(variable);
  }
}

proto::ValueStream &DiskDatastore::GetVariable(const string &variable) {
  MapType::iterator it = live_data_.find(variable);
  if (it == live_data_.end())
    throw out_of_range("Variable not found");
  return it->second;
}

proto::ValueStream &DiskDatastore::CreateVariable(const string &variable) {
  proto::ValueStream stream;
  stream.set_variable(variable);
  live_data_[variable] = stream;
  return live_data_[variable];
}

proto::Value *DiskDatastore::RecordNoLog(const string &variable, Timestamp timestamp, double value) {
  proto::ValueStream &stream = GetOrCreateVariable(variable);
  proto::Value *val = stream.add_value();
  val->set_timestamp(timestamp.ms());
  val->set_value(value);
  return val;
}

void DiskDatastore::ReplayRecordLog() {
  try {
    VLOG(1) << "Replaying record log";
    proto::ValueStream stream;
    uint64_t num_points = 0, num_streams = 0;
    while (record_log_.ReplayLog(&stream)) {
      for (int i = 0; i < stream.value_size(); i++) {
        RecordNoLog(stream.variable(), stream.value(i).timestamp(), stream.value(i).value());
        num_points++;
      }
      num_streams++;
    }
    LOG(INFO) << "Replayed record log, got " << num_points << " points" << " in " << num_streams << " streams";
  } catch (exception &e) {
    LOG(WARNING) << "Couldn't replay record log: " << e.what();
  }
}

}  // namespace
