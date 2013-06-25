/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>
#include <string>
#include <set>
#include "lib/common.h"
#include "lib/openinstrument.pb.h"
#include "lib/protobuf.h"
#include "lib/store_client.h"
#include "lib/store_config.h"
#include "lib/string.h"

using namespace openinstrument;
using namespace std;

void usage(char *argv[]) {
  cerr << "\nUsage: \n"
       << "\t" << argv[0] << " <server:port>\n\n"
       << "\n";
  exit(1);
}

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  if (argc != 2) {
    cerr << "Wrong number of arguments\n";
    usage(argv);
  }

  proto::StoreConfig config;

  try {
    StoreClient client(argv[1]);
    scoped_ptr<proto::StoreConfig> response(client.GetStoreConfig());
    StoreConfig::get_manager().HandleNewConfig(*response);
    cout << StoreConfig::get_manager().DumpConfig() << "\n";

  } catch (exception &e) {
    cerr << e.what();
    usage(argv);
    return 1;
  }

  return 0;
}
