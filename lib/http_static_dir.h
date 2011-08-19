/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef OPENINSTRUMENT_LIB_HTTP_STATIC_DIR_H_
#define OPENINSTRUMENT_LIB_HTTP_STATIC_DIR_H_

#include <string>
#include "lib/common.h"
#include "lib/mime_types.h"

namespace openinstrument {
namespace http {

class HttpServer;
class HttpRequest;
class HttpReply;

class HttpStaticHandler {
 public:
  HttpStaticHandler() {}
 protected:
  void ServeLocalFile(const string &filename, HttpReply *reply) const;
};

class HttpStaticDir : public HttpStaticHandler {
 public:
  HttpStaticDir(const string &http_path, const string &local_path, HttpServer *server);
  bool HandleGet(const HttpRequest &request, HttpReply *reply);

 private:
  void GetComplete(void *ptr, uint64_t size);

  string http_path_;
  string local_path_;
  HttpServer *server_;
};

class HttpStaticFile : public HttpStaticHandler {
 public:
  HttpStaticFile(const string &http_path, const string &local_path, HttpServer *server);
  bool HandleGet(const HttpRequest &request, HttpReply *reply);

 private:
  string http_path_;
  string local_path_;
  HttpServer *server_;
};

}  // namespace http
}  // namespace openinstrument

#endif  // OPENINSTRUMENT_LIB_HTTP_STATIC_DIR_H_
