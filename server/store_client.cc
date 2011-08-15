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
#include "lib/atomic.h"
#include "lib/common.h"
#include "lib/http_client.h"
#include "lib/socket.h"
#include "lib/string.h"
#include "lib/timer.h"

DEFINE_int32(port, 8020, "Port to connect to");
DEFINE_string(address, "127.0.0.1", "Address to connect to");

using namespace openinstrument;
using namespace openinstrument::http;

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  /*
  HttpClient client;
  Uri uri;
  uri.set_hostname(FLAGS_address);
  uri.set_port(FLAGS_port);
  uri.set_path("/list/openinstrument");
  scoped_ptr<HttpRequest> request(client.NewRequest(uri.Assemble()));
  scoped_ptr<HttpReply> reply(client.SendRequest(*request));
  LOG(INFO) << reply->status();
  LOG(INFO) << reply->content.ToString();
  */

  return 0;
}
