/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <string>
#include "lib/common.h"
#include "lib/http_message.h"
#include "lib/http_reply.h"
#include "lib/socket.h"

namespace openinstrument {
namespace http {

string HttpReply::StatusToResponse(HttpReply::status_type status) const {
  switch (status) {
    case HttpReply::OK:
      return StringPrintf("%s 200 OK\r\n", http_version().c_str());
    case HttpReply::CREATED:
      return StringPrintf("%s 201 Created\r\n", http_version().c_str());
    case HttpReply::ACCEPTED:
      return StringPrintf("%s 202 Accepted\r\n", http_version().c_str());
    case HttpReply::NO_CONTENT:
      return StringPrintf("%s 204 No Content\r\n", http_version().c_str());
    case HttpReply::MULTIPLE_CHOICES:
      return StringPrintf("%s 300 Multiple Choices\r\n", http_version().c_str());
    case HttpReply::MOVED_PERMANENTLY:
      return StringPrintf("%s 301 Moved Permanently\r\n", http_version().c_str());
    case HttpReply::MOVED_TEMPORARILY:
      return StringPrintf("%s 302 Moved Temporarily\r\n", http_version().c_str());
    case HttpReply::NOT_MODIFIED:
      return StringPrintf("%s 304 Not Modified\r\n", http_version().c_str());
    case HttpReply::BAD_REQUEST:
      return StringPrintf("%s 400 Bad Request\r\n", http_version().c_str());
    case HttpReply::UNAUTHORIZED:
      return StringPrintf("%s 401 Unauthorized\r\n", http_version().c_str());
    case HttpReply::FORBIDDEN:
      return StringPrintf("%s 403 Forbidden\r\n", http_version().c_str());
    case HttpReply::NOT_FOUND:
      return StringPrintf("%s 404 Not Found\r\n", http_version().c_str());
    case HttpReply::NOT_IMPLEMENTED:
      return StringPrintf("%s 501 Not Implemented\r\n", http_version().c_str());
    case HttpReply::BAD_GATEWAY:
      return StringPrintf("%s 502 Bad Gateway\r\n", http_version().c_str());
    case HttpReply::SERVICE_UNAVAILABLE:
      return StringPrintf("%s 503 Service Unavailable\r\n", http_version().c_str());
    case HttpReply::INTERNAL_SERVER_ERROR:
    default:
      return StringPrintf("%s 500 Internal Server Error\r\n", http_version().c_str());
  }
}

  // Get a stock reply.
void HttpReply::StockReply(HttpReply::status_type status) {
  status_ = status;
  switch (status) {
    case HttpReply::OK:
      break;
    case HttpReply::CREATED:
      body_ = "<html><head><title>Created</title></head><body><h1>201 Created</h1></body></html>";
      break;
    case HttpReply::ACCEPTED:
      body_ = "<html><head><title>Accepted</title></head><body><h1>202 Accepted</h1></body></html>";
      break;
    case HttpReply::NO_CONTENT:
      body_ = "<html><head><title>No Content</title></head><body><h1>204 Content</h1></body></html>";
      break;
    case HttpReply::MULTIPLE_CHOICES:
      body_ = "<html><head><title>Multiple Choices</title></head><body><h1>300 Multiple Choices</h1></body></html>";
      break;
    case HttpReply::MOVED_PERMANENTLY:
      body_ = "<html><head><title>Moved Permanently</title></head><body><h1>301 Moved Permanently</h1></body></html>";
      break;
    case HttpReply::MOVED_TEMPORARILY:
      body_ = "<html><head><title>Moved Temporarily</title></head><body><h1>302 Moved Temporarily</h1></body></html>";
      break;
    case HttpReply::NOT_MODIFIED:
      body_ = "<html><head><title>Not Modified</title></head><body><h1>304 Not Modified</h1></body></html>";
      break;
    case HttpReply::BAD_REQUEST:
      body_ = "<html><head><title>Bad Request</title></head><body><h1>400 Bad Request</h1></body></html>";
      break;
    case HttpReply::UNAUTHORIZED:
      body_ = "<html><head><title>Unauthorized</title></head><body><h1>401 Unauthorized</h1></body></html>";
      break;
    case HttpReply::FORBIDDEN:
      body_ = "<html><head><title>Forbidden</title></head><body><h1>403 Forbidden</h1></body></html>";
      break;
    case HttpReply::NOT_FOUND:
      body_ = "<html><head><title>Not Found</title></head><body><h1>404 Not Found</h1></body></html>";
      break;
    case HttpReply::NOT_IMPLEMENTED:
      body_ = "<html><head><title>Not Implemented</title></head><body><h1>501 Not Implemented</h1></body></html>";
      break;
    case HttpReply::BAD_GATEWAY:
      body_ = "<html><head><title>Bad Gateway</title></head><body><h1>502 Bad Gateway</h1></body></html>";
      break;
    case HttpReply::SERVICE_UNAVAILABLE:
      body_ = "<html><head><title>Service Unavailable</title></head><body><h1>503 Service Unavailable</h1>"
              "</body></html>";
      break;
    case HttpReply::INTERNAL_SERVER_ERROR:
    default:
      body_ = "<html><head><title>Internal Server Error</title></head><body><h1>500 Internal Server Error"
              "</h1></body></html>";
      break;
  }
  SetContentLength(body_.size());
  SetContentType("text/html; charset=UTF-8");
}

void HttpReply::WriteFirstline(Socket *sock) {
  VLOG(2) << StatusToResponse(status_);
  sock->Write(StatusToResponse(status_));
}

}  // namespace http
}  // namespace openinstrument
