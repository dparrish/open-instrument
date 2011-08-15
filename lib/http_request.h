/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_HTTP_REQUEST_
#define _OPENINSTRUMENT_LIB_HTTP_REQUEST_

#include <string>
#include "lib/http_message.h"
#include "lib/socket.h"
#include "lib/uri.h"

namespace openinstrument {
namespace http {

// A request received from a client.
class HttpRequest : public HttpMessage {
 public:
  HttpRequest() {}
  bool HasParam(const string &key) const;
  const string &GetParam(const string &key) const;

  string url;
  Uri uri;
  Socket::Address source;

 private:
  void WriteFirstline(Socket *sock);
};

}  // namespace http
}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_HTTP_REQUEST_
