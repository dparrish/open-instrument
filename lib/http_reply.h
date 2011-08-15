/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_HTTP_REPLY_H_
#define _OPENINSTRUMENT_LIB_HTTP_REPLY_H_

#include <string>
#include "lib/closure.h"
#include "lib/common.h"
#include "lib/http_message.h"

namespace openinstrument {
class Socket;

namespace http {

// A reply to be sent to a client.
class HttpReply : public HttpMessage {
 public:
  HttpReply() : status_(INTERNAL_SERVER_ERROR), complete_callback_(NULL) {}
  HttpReply(const HttpReply &copy) : HttpMessage(copy), status_(copy.status_) {}

  virtual ~HttpReply() {
    if (complete_callback_)
      complete_callback_();
  }

  enum status_type {
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NO_CONTENT = 204,
    MULTIPLE_CHOICES = 300,
    MOVED_PERMANENTLY = 301,
    MOVED_TEMPORARILY = 302,
    NOT_MODIFIED = 304,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SERVICE_UNAVAILABLE = 503
  };

  string StatusToResponse(status_type status) const;

  // Get a stock reply.
  void StockReply(status_type status);

  inline bool IsSuccess() const {
    if (status_ < BAD_REQUEST)
      return true;
    return false;
  }

  inline status_type status() const {
    return status_;
  }

  inline bool success() {
    return static_cast<uint16_t>(status_) >= 200 && static_cast<uint16_t>(status_) < 400;
  }

  inline bool failure() {
    return static_cast<uint16_t>(status_) >= 400;
  }

  inline void set_status(status_type status) {
    status_ = status;
  }

  void AddCompleteCallback(Callback done) {
    complete_callback_ = done;
  }

 protected:
  // The status of the reply.
  status_type status_;
  Callback complete_callback_;

  void WriteFirstline(Socket *sock);

  friend class HttpClient;
};

}  // namespace http
}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_HTTP_REPLY_H_
