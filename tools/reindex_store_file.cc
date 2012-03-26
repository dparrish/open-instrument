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
#include "server/disk_datastore.h"
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

  log.ReindexRecordLogFile(argv[1], &log_data);
  log.WriteIndexedFile(log_data, argv[1]);

  DiskDatastore store("NULL DIRECTORY");
  IndexedStoreFile file(argv[1]);
  vector<proto::ValueStream> streams = file.GetVariable(Variable("/network/device/configuration"));

  //rename(outfile.c_str(), argv[1]);
  return 0;
}
