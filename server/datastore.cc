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
#include "lib/string.h"
#include "lib/timer.h"
#include "server/datastore.h"

DECLARE_string(datastore);

namespace openinstrument {

set<IndexedStoreFile> store_set;

proto::ValueStream &IndexedStoreFile::LoadAndGetVar(const string &variable, uint64_t timestamp) {
  vector<string> files = Glob(StringPrintf("%s/datastore.*.bin", FLAGS_datastore.c_str()));
  // Look for the oldest file that may contain the desired timestamp
  for (vector<string>::iterator i = files.begin(); i != files.end(); ++i) {
    string filename = *i;
    size_t timepos = filename.find(".");
    if (timepos == string::npos)
      continue;
    string tmp = filename.substr(timepos + 1);
    size_t extpos = tmp.find(".");
    if (extpos == string::npos)
      continue;
    uint64_t ts;
    try {
      ts = lexical_cast<uint64_t>(tmp.substr(0, extpos));
      if (timestamp > ts)
        continue;
    } catch (exception &e) {
      continue;
    }
    IndexedStoreFile isf(filename);
    if (!isf.ReadHeader())
      throw runtime_error("Error reading logfile header");
    if (isf.file_header_.start_timestamp() > timestamp)
      throw runtime_error(StringPrintf("Data file %s does not contain old enough data", filename.c_str()));
    if (!isf.ContainsVariable(variable))
      throw runtime_error(StringPrintf("Data file %s does not contain variable", filename.c_str()));
    // Found the correct file, it has a valid index and the variable's stream exists, return it.
    return isf.LoadVariable(variable);
  }
  throw out_of_range("Could not find log file containing requested data");
}

void IndexedStoreFile::Clear() {
  log_data_.clear();
}

bool IndexedStoreFile::ReadHeader() {
  Clear();
  fh_.SeekAbs(0);
  ProtoStreamReader reader(filename_);
  if (!reader.Next(&file_header_)) {
    LOG(ERROR) << "Error reading header from " << filename_;
    return false;
  }
  for (int i = 0; i< file_header_.variable_size(); i++)
    log_data_[file_header_.variable(i)] = proto::ValueStream();
  return true;
}

proto::ValueStream &IndexedStoreFile::LoadVariable(const string &variable) {
  MapType::iterator i = log_data_.find(variable);
  if (i == log_data_.end())
    throw out_of_range("Variable not found");
  if (i->second.value_size())
    return i->second;
  ProtoStreamReader reader(filename_);
  for (int i = 0; i < file_header_.variable_size(); i++) {
    reader.Skip(1);
    if (file_header_.variable(i) == variable) {
      proto::ValueStream &stream = log_data_[variable];
      if (!reader.Next(&stream)) {
        throw runtime_error("Unable to read protobuf from stream");
      }
      if (stream.variable() != variable) {
        LOG(WARNING) << "Stream variable " << i << " " << stream.variable() << " in file " << filename_
                     << " is not correct";
        throw out_of_range("Variable unable to be loaded");
      }
      return log_data_[variable];
    }
  }
  throw out_of_range("Variable does not exist in log file");
}

}  // namespace openinstrument
