/*
 * Recreate a datastore file.
 *
 * Supply a single filename on the commandline.
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <iostream>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include "lib/common.h"
#include "lib/openinstrument.pb.h"
#include "lib/protobuf.h"
#include "lib/string.h"
#include "lib/timer.h"
#include "lib/variable.h"
#include "server/record_log.h"

using namespace openinstrument;
using namespace std;

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  if (argc != 2) {
    cerr << "You must specify a single filename\n";
    return 1;
  }

  // Create the RecordLog reader. This must be given a base directory, but this
  // shouldn't be used unless an entry is added.
  RecordLog log("NULL DIRECTORY");
  RecordLog::MapType log_data;

  uint64_t output_streams = 0, output_values = 0;
  proto::StoreFileHeader header;
  log.ReindexRecordLogFile(argv[1], &log_data);

  // Build the output header
  for (RecordLog::MapType::iterator i = log_data.begin(); i != log_data.end(); ++i) {
    proto::StreamVariable *var = header.add_variable();
    var->CopyFrom(i->variable());
    output_values += i->value_size();
    output_streams++;
    if (i->value_size()) {
      const proto::Value &first_value = i->value(0);
      if (!header.has_start_timestamp() || first_value.timestamp() < header.start_timestamp())
        header.set_start_timestamp(first_value.timestamp());
      const proto::Value &last_value = i->value(i->value_size() - 1);
      if (last_value.timestamp() > header.end_timestamp())
        header.set_end_timestamp(last_value.timestamp());
      if (last_value.end_timestamp() > header.end_timestamp())
        header.set_end_timestamp(last_value.end_timestamp());
    }
  }

  string outfile = StringPrintf("%s.new", argv[1]);
  FileStat stat(outfile);
  if (stat.exists())
    LOG(FATAL) << "Output file " << outfile << " already exists";

  // Write the header and all ValueStreams
  ProtoStreamWriter writer(outfile);
  writer.Write(header);
  for (RecordLog::MapType::iterator i = log_data.begin(); i != log_data.end(); ++i) {
    writer.Write(*i);
  }
  LOG(INFO) << "Created indexed file " << outfile << " containing " << output_streams << " streams and "
            << output_values << " values, between " << Timestamp(header.start_timestamp()).GmTime() << " and "
            << Timestamp(header.end_timestamp()).GmTime();

  rename(outfile.c_str(), argv[1]);
  return 0;
}
