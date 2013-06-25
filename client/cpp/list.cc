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

DEFINE_int32(max_variables, 100, "Maximum number of variables to return");

void usage(char *argv[]) {
  cerr << "\nUsage: \n"
       << "\t" << argv[0] << " <server:port> <prefix>\n\n"
       << "  <prefix> should be of the form \"/var/name/here\".\n"
       << "\n";
  exit(1);
}

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  if (argc != 3) {
    cerr << "Wrong number of arguments\n";
    usage(argv);
  }

  // Retrieve cluster config from the single server specified on the commandline.
  StoreConfig &config = StoreConfig::get_manager();
  try {
    StoreClient client(argv[1]);
    scoped_ptr<proto::StoreConfig> response(client.GetStoreConfig());
    config.HandleNewConfig(*response);
  } catch (exception &e) {
    cerr << "Could not retrieve cluster config from " << argv[1] << ": " << e.what() << "\n";
    usage(argv);
    return 1;
  }

  // Build the List request and send it to the entire cluster.
  try {
    proto::ListRequest req;
    req.set_max_variables(FLAGS_max_variables);
    Variable(argv[2]).ToProtobuf(req.mutable_prefix());
    StoreClient client;
    scoped_ptr<proto::ListResponse> response(client.List(req));
    set<string> variables;
    for (auto &stream : response->stream()) {
      Variable var(stream.variable());
      variables.insert(var.ToString());
    }
    for (string var : variables)
      cout << var << endl;
  } catch (exception &e) {
    cerr << e.what();
    usage(argv);
    return 1;
  }

  return 0;
}
