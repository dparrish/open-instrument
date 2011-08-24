/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef OPENINSTRUMENT_LIB_HTTP_REQUEST_HANDLER_H_
#define OPENINSTRUMENT_LIB_HTTP_REQUEST_HANDLER_H_

#include <string>
#include <vector>
#include "lib/common.h"
#include "lib/http_headers.h"
#include "lib/http_reply.h"
#include "lib/http_request.h"
#include "lib/uri.h"

namespace openinstrument {
namespace http {

// The common handler for all incoming requests.
class RequestHandler : private noncopyable {
 public:
  // Construct with a directory containing files to be served.
  RequestHandler() {}
  virtual ~RequestHandler() {}

  // Handle a request by forwarding it on to an appropriate callback.
  virtual void HandleRequest(const HttpRequest &req, HttpReply *reply) {
    VLOG(1) << "Received request for \"" << req.uri.Assemble() << "\"";
    for (size_t i = 0; i < req.headers().size(); ++i) {
      VLOG(2) << "\t\"" << req.headers()[i].name << "\": \"" << req.headers()[i].value << "\"";
    }

    // Find a callback for the requested path
    for (size_t i = 0; i < handlers_.size(); ++i) {
      if (boost::ends_with(handlers_[i].path, "$")) {
        // Require full match
        string fullpath = handlers_[i].path.substr(0, handlers_[i].path.size() - 1);
        if (req.uri.path != fullpath)
          continue;
      } else {
        // Just substring match
        if (!boost::starts_with(req.uri.path, handlers_[i].path))
          continue;
      }

      // Run the callback
      reply->SetStatus(HttpReply::OK);
      reply->SetHeader("X-Frame-Options", "SAMEORIGIN");
      try {
        if (!handlers_[i].callback(req, reply)) {
          LOG(WARNING) << "Callback for " << req.uri.path << " returned false";
          reply->StockReply(HttpReply::INTERNAL_SERVER_ERROR);
        }
      } catch (exception &e) {
        LOG(ERROR) << "Callback for " << req.uri.path << " threw an exception: " << e.what();
        reply->StockReply(HttpReply::INTERNAL_SERVER_ERROR);
      }
      return;
    }

    LOG(INFO) << "Could not find handler for path \"" << req.uri.path << "\", returning 404";
    reply->StockReply(HttpReply::NOT_FOUND);
  }

  // Add a handler callback for a specific path.
  //
  // This takes a string to match against incoming request paths. If this string ends in "$", a full string match will
  // be done, otherwise any path that begins in this string will match.
  //
  // The second argumet is a pointer to a member function that will handle the request, which is of the form:
  //   bool callback_function(const HttpRequest &request, HttpReply *reply)
  //
  // The third argument is the object containing the member function.
  //
  // The callback should return true if the request was successfully handled.  If it returns false or throws an
  // exception, a 500 error will be returned to the user.
  //
  // Sample usage:
  //   server->request_handler()->add_path("/test$", &TestServer::handle_test, this);
  template<class T>
  void AddPath(const string &path, bool (T::*func)(const HttpRequest &, HttpReply *), T *obj) {
    Handler h;
    h.path = path;
    h.callback = bind(func, obj, _1, _2);
    handlers_.push_back(h);
  }

 private:
  struct Handler {
    string path;
    boost::function<bool(const HttpRequest &, HttpReply *)> callback;
  };
  vector<Handler> handlers_;
};

}  // namespace http
}  // namespace openinstrument

#endif  // OPENINSTRUMENT_LIB_HTTP_REQUEST_HANDLER_H_
