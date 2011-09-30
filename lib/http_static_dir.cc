/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <string>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include "lib/common.h"
#include "lib/file.h"
#include "lib/http_reply.h"
#include "lib/http_request.h"
#include "lib/http_server.h"
#include "lib/http_static_dir.h"
#include "lib/mime_types.h"

namespace openinstrument {
namespace http {

scoped_ptr<MimeTypes> http_static_mime_types_(NULL);

void ReadMimeTypes() {
  if (http_static_mime_types_.get())
    return;
  http_static_mime_types_.reset(new MimeTypes());
  http_static_mime_types_->ReadFile("/etc/mime.types");
}

const string &MimeType(const string &filename) {
  ReadMimeTypes();
  return http_static_mime_types_->Lookup(filename);
}

void HttpStaticHandler::ServeLocalFile(const string &filename, const HttpRequest &request, HttpReply *reply) const {
  if (request.headers().HeaderExists("If-Modified-Since")) {
    Timestamp ifs(Timestamp::FromGmTime(request.headers().GetHeader("If-Modified-Since"), HttpServer::RFC112Format));
    FileStat fs(filename);
    if (fs.mtime() <= ifs.seconds()) {
      reply->SetStatus(HttpReply::NOT_MODIFIED);
      reply->SetHeader("Last-Modified", Timestamp(fs.mtime() * 1000).GmTime(HttpServer::RFC112Format).c_str());
      reply->SetContentLength(0);
      return;
    }
  }

  VLOG(1) << "Serving local file " << filename;
  File fh(filename, "r");
  reply->SetStatus(HttpReply::OK);
  reply->SetContentLength(fh.stat().size());
  reply->SetContentType(MimeType(filename).c_str());
  reply->SetHeader("Last-Modified", Timestamp(fh.stat().mtime() * 1000).GmTime(HttpServer::RFC112Format).c_str());
  for (int64_t done = 0; done < fh.stat().size(); ) {
    uint32_t avail = 4096;
    char *buf;
    if (fh.stat().size() - done < 4096)
      avail = fh.stat().size() - done;
    reply->mutable_body()->GetAppendBuf(avail, &buf, &avail);
    if (!avail)
      continue;
    int32_t readbytes = fh.Read(buf, avail);
    if (readbytes < 0)
      continue;
    if (readbytes == 0)
      break;
    done += readbytes;
  }
}

HttpStaticDir::HttpStaticDir(const string &http_path, const string &local_path, HttpServer *server)
  : HttpStaticHandler(),
    http_path_(http_path),
    local_path_(local_path),
    server_(server) {
  server_->request_handler()->AddPath(http_path_, &HttpStaticDir::HandleGet, this);
}

void HttpStaticDir::GetComplete(void *ptr, uint64_t size) {
  CHECK(ptr);
  CHECK(size);
  munmap(ptr, size);
}

bool HttpStaticDir::HandleGet(const HttpRequest &request, HttpReply *reply) {
  string localfile = request.uri.path;
  if (request.uri.path.find(http_path_) != 0) {
    LOG(WARNING) << "Could not find http path in request for " << localfile;
    reply->SetStatus(HttpReply::NOT_FOUND);
    return true;
  }
  localfile.replace(0, http_path_.size(), local_path_);
  if (localfile.find(string("\0", 1)) != string::npos) {
    LOG(WARNING) << "NULL found in request for local file at " << localfile.find("\0");
    reply->SetStatus(HttpReply::NOT_FOUND);
    return true;
  }
  while (localfile[0] == '/' || localfile[0] == '.')
    localfile.erase(0, 1);
  if (localfile.find("../") != string::npos) {
    LOG(WARNING) << ".. found in request for local file";
    reply->SetStatus(HttpReply::NOT_FOUND);
    return true;
  }

  try {
    ServeLocalFile(localfile, request, reply);
  } catch (runtime_error &e) {
    LOG(ERROR) << e.what();
    reply->SetStatus(HttpReply::NOT_FOUND);
  }
  return true;
}


HttpStaticFile::HttpStaticFile(const string &http_path, const string &local_path, HttpServer *server)
  : HttpStaticHandler(),
    http_path_(http_path),
    local_path_(local_path),
    server_(server) {
  string path = http_path;
  if (path.at(path.size() - 1) != '$')
    path += '$';
  server_->request_handler()->AddPath(path, &HttpStaticFile::HandleGet, this);
}

bool HttpStaticFile::HandleGet(const HttpRequest &request, HttpReply *reply) {
  if (request.uri.path != http_path_) {
    LOG(WARNING) << "Could not find http path in request for " << http_path_;
    reply->SetStatus(HttpReply::NOT_FOUND);
    return true;
  }

  try {
    ServeLocalFile(local_path_, request, reply);
  } catch (runtime_error &e) {
    LOG(ERROR) << e.what();
    reply->SetStatus(HttpReply::NOT_FOUND);
  }
  return true;
}

}  // namespace http
}  // namespace openinstrument
