/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef OPENINSTRUMENT_LIB_HTTP_HTTPCLIENT_
#define OPENINSTRUMENT_LIB_HTTP_HTTPCLIENT_

#include <string>
#include "lib/common.h"
#include "lib/http_reply.h"
#include "lib/http_request.h"
#include "lib/socket.h"
#include "lib/string.h"
#include "lib/timer.h"
#include "lib/uri.h"

namespace openinstrument {
namespace http {

class HttpClient : private noncopyable {
 public:
  HttpClient() : deadline_time_(30000) {
  }

  HttpReply *SendRequest(HttpRequest &request);
  HttpRequest *NewRequest(const string &url);

  inline void set_deadline(uint64_t deadline) {
    deadline_time_ = deadline;
  }

 private:
  uint64_t deadline_time_;
};

}  // namespace http
}  // namespace openinstrument

#endif  // OPENINSTRUMENT_LIB_HTTP_HTTPCLIENT_H_
