/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef OPENINSTRUMENT_LIB_HTTP_MESSAGE_H_
#define OPENINSTRUMENT_LIB_HTTP_MESSAGE_H_

#include <string>
#include "lib/common.h"
#include "lib/cord.h"
#include "lib/http_headers.h"
#include "lib/string.h"
#include "lib/timer.h"

namespace openinstrument {
class Socket;

namespace http {

class HttpMessage {
 public:
  HttpMessage()
    : method_("GET"),
      header_written_(false),
      status_written_(false) {}
  virtual ~HttpMessage() {}

  void WriteHeader(Socket *sock);
  void set_http_version(const string &version);
  string http_version() const;
  void SetHeader(const char *key, const char *value);
  const HttpHeaders &headers() const;
  void SetContentLength(size_t length = 0);
  uint64_t GetContentLength() const;
  void SetContentType(const char *type);
  const string &GetContentType();
  void Write(Socket *sock);
  void ReadAndParseHeaders(Socket *sock, Deadline deadline);

  const Cord &body() const {
    return body_;
  }

  Cord *mutable_body() {
    return &body_;
  }

  const string &method() const {
    return method_;
  }

  void set_method(const string &method) {
    method_ = method;
  }

 protected:
  string method_;
  uint8_t version_major_;
  uint8_t version_minor_;
  bool header_written_;
  bool status_written_;
  HttpHeaders headers_;

  Cord body_;

  // Override this for subclasses. This is the first line that is written.
  virtual void WriteFirstline(Socket *sock) = 0;

  inline HttpHeaders *mutable_headers() {
    return &headers_;
  }

  static const char *crlf_;
  static const char *header_sep_;

  friend class HttpServer;
  friend class HttpClient;
};

}  // namespace http
}  // namespace openinstrument

#endif  // OPENINSTRUMENT_LIB_HTTP_MESSAGE_H_
