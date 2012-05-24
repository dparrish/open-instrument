/*
 * Dump a recordlog file to stdout.
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
  log.AddLogFile(argv[1]);

  proto::ValueStream stream;
  while (log.ReplayLog(&stream)) {
    Variable var;
    var.FromValueStream(stream);
    // TODO(dparrish): Remove this once DEPRECATED_string_variable has been removed
    if (stream.has_deprecated_string_variable()) {
      var.FromString(stream.deprecated_string_variable());
      cout << "old style variable name\n";
    }
    if (var.ToString().empty()) {
      cout << "EMPTY VARIABLE\t";
    } else {
      cout << var.ToString() << "\t";
    }
    for (auto &value : stream.value()) {
      cout << StringPrintf("%s\t", Timestamp(value.timestamp()).GmTime().c_str());
      if (value.end_timestamp())
        cout << StringPrintf(" - %s\t", Timestamp(value.end_timestamp()).GmTime().c_str());
      if (value.has_double_value()) {
        cout << "DOUBLE:" << fixed << value.double_value();
        if (value.has_string_value())
          cout << "\t";
      }
      if (value.has_string_value())
        cout << "STRING:" << value.string_value();
      cout << "\n";
    }

  }

  return 0;
}
