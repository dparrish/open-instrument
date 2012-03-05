/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <boost/asio/ip/host_name.hpp>
#include <boost/regex.hpp>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>
#include <string>
#include "lib/common.h"
#include "lib/openinstrument.pb.h"
#include "lib/protobuf.h"
#include "lib/socket.h"
#include "lib/store_client.h"
#include "lib/string.h"

DEFINE_string(hostname, "", "Hostname to send to the server (source host for the variable)");

using namespace openinstrument;
using namespace std;

void usage(char *argv[]) {
  cerr << "\nUsage: \n"
       << "\t" << argv[0] << " [--hostname=<hostname>] <server:port> <variable>:<value>[@<timestamp>]...\n\n"
       << "  <variable> must be of the form \"/var/name/here\".\n"
       << "  <value> must be a floating point number.\n"
       << "\n";
  exit(1);
}

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  if (argc < 3) {
    cerr << "Wrong number of arguments\n";
    usage(argv);
  }

  string hostname(boost::asio::ip::host_name());
  if (!FLAGS_hostname.empty())
    hostname = FLAGS_hostname;

  // Build a single AddRequest containing all variables supplied on the commandline
  proto::AddRequest req;
  boost::regex regex("(.+):([\\d\\.]+)(?:@(\\d+))?");
  for (int i = 2; i < argc; i++) {
    boost::smatch what;
    string foo(argv[i]);
    if (!boost::regex_match(foo, what, regex) || what.size() < 3) {
      cerr << "Invalid variable";
      usage(argv);
    }

    proto::ValueStream *stream = req.add_stream();
    Variable var(what[1]);
    if (!var.HasLabel("srchost"))
      var.SetLabel("srchost", hostname);
    var.ToValueStream(stream);

    proto::Value *value = stream->add_value();
    try {
      value->set_double_value(lexical_cast<double>(what[2]));
    } catch (exception) {
      cerr << "Invalid value " << what[2] << "\n";
      usage(argv);
    }
    try {
      if (what.size() == 4 && what[3] != "")
        value->set_timestamp(lexical_cast<uint64_t>(what[3]));
      else
        value->set_timestamp(Timestamp::Now());
    } catch (exception) {
      cerr << "Invalid timestamp " << what[3] << "\n";
      usage(argv);
    }
  }

  try {
    StoreClient client(argv[1]);
    scoped_ptr<proto::AddResponse> response(client.Add(req));
  } catch (exception &e) {
    cerr << e.what() << std::endl;
    return 1;
  }

  return 0;
}
