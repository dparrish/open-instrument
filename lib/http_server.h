/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_HTTPSERVER_SERVER_
#define _OPENINSTRUMENT_LIB_HTTPSERVER_SERVER_

#include <boost/thread.hpp>
#include <signal.h>
#include <string>
#include <vector>
#include "lib/common.h"
#include "lib/exported_vars.h"
#include "lib/request_handler.h"
#include "lib/socket.h"

namespace openinstrument {
namespace http {

// The top-level class of the HTTP server.
class HttpServer : private noncopyable {
 public:
  // Construct the server to listen on the specified TCP address and port, with a given thread pool size.
  explicit HttpServer(const string &address, const uint16_t port, Executor *executor);

  ~HttpServer();

  // Start listening on the specified host and port.
  void Listen();

  // Run the event loop.
  void Start();

  // Stop the server.
  void Stop();

  // Use a given RequestHandler object to handle requests.
  // This class takes ownership of the handler object.
  void set_request_handler(RequestHandler *handler) {
    request_handler_.reset(handler);
  }

  RequestHandler *request_handler();
  thread *listen_thread() const;
  void AddExportHandler();

  static const char *RFC112Format;

 private:
  void Acceptor();
  bool HandleExportVars(const HttpRequest &request, HttpReply *reply);
  void HandleClient(Socket *sock);

  Socket::Address address_;
  Socket listen_socket_;
  scoped_ptr<thread> listen_thread_;

  Executor *executor_;

  // The handler for all incoming requests.
  scoped_ptr<RequestHandler> request_handler_;

  ExportedIntegerSet stats_;

  // Set to true to cause all the listening threads to shut down once they complete the in-progress request
  bool shutdown_;
};

}  // namespace http
}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_HTTPSERVER_SERVER_
