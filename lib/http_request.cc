/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <string>
#include <vector>
#include "lib/http_headers.h"
#include "lib/http_message.h"
#include "lib/http_request.h"
#include "lib/socket.h"
#include "lib/uri.h"

namespace openinstrument {
namespace http {

bool HttpRequest::HasParam(const string &key) const {
  for (vector<Uri::RequestParam>::const_iterator i = uri.params.begin(); i != uri.params.end(); ++i) {
    if (i->key == key)
      return true;
  }
  return false;
}

const string &HttpRequest::GetParam(const string &key) const {
  static const string empty_string("");
  for (vector<Uri::RequestParam>::const_iterator i = uri.params.begin(); i != uri.params.end(); ++i) {
    if (i->key == key)
      return i->value;
  }
  return empty_string;
}

void HttpRequest::WriteFirstline(Socket *sock) {
  Cord firstline;
  firstline.CopyFrom(StringPrintf("%s %s %s\r\n", method().c_str(), uri.Assemble().c_str(),
                                  http_version().c_str()));
  sock->Write(firstline);
  sock->Flush();
}

}  // namespace http
}  // namespace openinstrument
