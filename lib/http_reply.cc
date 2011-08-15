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
      return "HTTP/1.0 200 OK\r\n";
    case HttpReply::CREATED:
      return "HTTP/1.0 201 Created\r\n";
    case HttpReply::ACCEPTED:
      return "HTTP/1.0 202 Accepted\r\n";
    case HttpReply::NO_CONTENT:
      return "HTTP/1.0 204 No Content\r\n";
    case HttpReply::MULTIPLE_CHOICES:
      return "HTTP/1.0 300 Multiple Choices\r\n";
    case HttpReply::MOVED_PERMANENTLY:
      return "HTTP/1.0 301 Moved Permanently\r\n";
    case HttpReply::MOVED_TEMPORARILY:
      return "HTTP/1.0 302 Moved Temporarily\r\n";
    case HttpReply::NOT_MODIFIED:
      return "HTTP/1.0 304 Not Modified\r\n";
    case HttpReply::BAD_REQUEST:
      return "HTTP/1.0 400 Bad Request\r\n";
    case HttpReply::UNAUTHORIZED:
      return "HTTP/1.0 401 Unauthorized\r\n";
    case HttpReply::FORBIDDEN:
      return "HTTP/1.0 403 Forbidden\r\n";
    case HttpReply::NOT_FOUND:
      return "HTTP/1.0 404 Not Found\r\n";
    case HttpReply::NOT_IMPLEMENTED:
      return "HTTP/1.0 501 Not Implemented\r\n";
    case HttpReply::BAD_GATEWAY:
      return "HTTP/1.0 502 Bad Gateway\r\n";
    case HttpReply::SERVICE_UNAVAILABLE:
      return "HTTP/1.0 503 Service Unavailable\r\n";
    case HttpReply::INTERNAL_SERVER_ERROR:
    default:
      return "HTTP/1.0 500 Internal Server Error\r\n";
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
  sock->Write(StatusToResponse(status_));
}

}  // namespace http
}  // namespace openinstrument
