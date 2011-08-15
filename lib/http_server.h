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

  static const char *RFC112Format;

  // Construct the server to listen on the specified TCP address and port, and
  // serve up files from the given directory.
  explicit HttpServer(const string &address, const uint16_t port, size_t thread_pool_size)
    : address_(address),
      listen_thread_(NULL),
      thread_pool_size_(thread_pool_size),
      request_handler_(new RequestHandler()),
      stats_("/openinstrument/httpserver"),
      shutdown_(false) {
    address_.port = port;
  }

  ~HttpServer();

  // Run the event loop.
  void Start();

  // Stop the server.
  void Stop();

  RequestHandler *request_handler();
  thread *listen_thread() const;
  static HttpServer *NewServer(const string &addr, uint16_t port, size_t num_threads);
  void AddExportHandler();

 private:
  void Acceptor();
  bool HandleExportVars(const HttpRequest &request, HttpReply *reply);
  void HandleClient(Socket *sock);

  Socket::Address address_;
  Socket listen_socket_;
  scoped_ptr<thread> listen_thread_;

  // The number of threads that will call io_service::run().
  size_t thread_pool_size_;

  // The handler for all incoming requests.
  scoped_ptr<RequestHandler> request_handler_;

  ExportedIntegerSet stats_;

  // Set to true to cause all the listening threads to shut down once they complete the in-progress request
  bool shutdown_;
};

}  // namespace http
}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_HTTPSERVER_SERVER_
