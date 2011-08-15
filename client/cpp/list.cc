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
#include "lib/common.h"
#include "lib/openinstrument.pb.h"
#include "lib/protobuf.h"
#include "lib/store_client.h"
#include "lib/string.h"

using namespace openinstrument;
using namespace std;

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

  try {
    proto::ListRequest req;
    req.set_prefix(argv[2]);
    StoreClient client(argv[1]);
    scoped_ptr<proto::ListResponse> response(client.List(req));
    for (int i = 0; i < response->stream_size(); i++) {
      const proto::ValueStream &stream = response->stream(i);
      cout << stream.variable() << endl;
    }
  } catch (exception &e) {
    cerr << e.what();
    usage(argv);
    return 1;
  }

  return 0;
}
