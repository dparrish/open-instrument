/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include "lib/http_server.h"

namespace openinstrument {
namespace http {

const char *HttpServer::RFC112Format = "%a, %d %b %Y %H:%M:%S %Z";

HttpServer::HttpServer(const string &address, const uint16_t port, Executor *executor)
  : address_(address),
    listen_thread_(NULL),
    executor_(executor),
    request_handler_(new RequestHandler()),
    stats_("/openinstrument/httpserver"),
    shutdown_(false) {
  address_.port = port;
  Listen();
}

HttpServer::~HttpServer() {
  Stop();
}

void HttpServer::Listen() {
  listen_thread_.reset(new thread(bind(&HttpServer::Start, this)));
  listen_socket_.Listen(address_);
}

void HttpServer::Start() {
  // Block all signals
  sigset_t new_mask;
  sigfillset(&new_mask);
  sigset_t old_mask;
  pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

  LOG(INFO) << "HttpServer listening on " << address_.ToString();

  while (!shutdown_) {
    scoped_ptr<Socket> client(listen_socket_.Accept(1000));
    if (!client.get())
      continue;
    ++stats_["/connections-received"];
    VLOG(1) << "Accepted new connection from " << client->remote().ToString();
    try {
      executor_->Add(bind(&HttpServer::HandleClient, this, client.release()));
    } catch (exception &e) {
      LOG(WARNING) << e.what();
    }
  }
}

void HttpServer::Stop() {
  LOG(INFO) << "Stopping HttpServer";
  shutdown_ = true;
  if (listen_thread_.get())
    listen_thread_->join();
}

void HttpServer::HandleClient(Socket *client) {
  scoped_ptr<Socket> sock(client);
  bool close_connection = false;
  while (!close_connection) {
    Deadline deadline(30000);
    HttpRequest request;
    request.source = sock->remote();
    for (int i = 0; i < 20; ++i) {
      string output;
      try {
        sock->read_buffer()->ConsumeLine(&output);
      } catch (out_of_range) {
        try {
          if (sock->Read(deadline) == 0)
            return;
          continue;
        } catch (exception) {
          return;
        }
      }

      try {
        request.set_method(ConsumeFirstWord(&output));
        request.uri = Uri(ConsumeFirstWord(&output));
        request.set_http_version(ConsumeFirstWord(&output));
        if (request.http_version() == "HTTP/0.0")
          return;
        request.ReadAndParseHeaders(sock.get(), deadline);
      } catch (exception &e) {
        LOG(WARNING) << "Invalid HTTP request: " << e.what();
        HttpReply reply;
        reply.StockReply(HttpReply::BAD_REQUEST);
        reply.Write(sock.get());
        return;
      }
      break;
    }

    if (request.GetContentLength()) {
      // There has been some POST data
      while (sock->read_buffer()->size() < request.GetContentLength()) {
        try {
          if (sock->Read(deadline) == 0)
            return;
        } catch (exception) {
          return;
        }
      }
    } else if (request.method() == "POST") {
      if (request.headers().GetHeader("Connection") != "close") {
        LOG(WARNING) << "POST request with no Content-Length and Connection != close";
        close_connection = true;
      }
      while (sock->Read(deadline) > 0);
    }

    if (sock->read_buffer()->size())
      request.mutable_body()->CopyFrom(*sock->read_buffer());

    HttpReply reply;
    reply.set_http_version(request.http_version());

    if (request.http_version() == "HTTP/1.1" ||
        request.headers().GetHeader("Accept-Encoding").find("chunked") != string::npos) {
      reply.set_chunked_encoding(true);
    }

    request_handler_->HandleRequest(request, &reply);

    try {
      // Set default required headers in the reply
      if (reply.chunked_encoding()) {
        reply.mutable_headers()->RemoveHeader("Content-Length");
        reply.mutable_headers()->AddHeader("Transfer-Encoding", "chunked");
      } else {
        if (!reply.headers().HeaderExists("Content-Length"))
          reply.SetContentLength(reply.body().size());
        reply.mutable_headers()->RemoveHeader("Transfer-Encoding");
      }
      if (!reply.headers().HeaderExists("Content-Type"))
        reply.SetContentType("text/html; charset=UTF-8");
      if (!reply.headers().HeaderExists("Date"))
        reply.mutable_headers()->AddHeader("Date", Timestamp().GmTime(RFC112Format));
      if (!reply.headers().HeaderExists("Last-Modified"))
        reply.mutable_headers()->AddHeader("Last-Modified", Timestamp().GmTime(RFC112Format));
      if (!reply.headers().HeaderExists("Server"))
        reply.mutable_headers()->AddHeader("Server", "OpenInstrument/1.0");
      if (!reply.headers().HeaderExists("X-Frame-Options"))
        reply.mutable_headers()->AddHeader("X-Frame-Options", "SAMEORIGIN");
      if (!reply.headers().HeaderExists("X-XSS-Protection"))
        reply.mutable_headers()->AddHeader("X-XSS-Protection", "1; mode=block");
      if (!reply.headers().HeaderExists("X-Frame-Options"))
        reply.mutable_headers()->AddHeader("X-Frame-Options", "deny");
      if (!close_connection && request.http_version() >= "HTTP/1.1" &&
          request.headers().GetHeader("Connection").find("close") == string::npos) {
        reply.mutable_headers()->SetHeader("Connection", "keep-alive");
      } else {
        close_connection = true;
        reply.mutable_headers()->SetHeader("Connection", "close");
      }
    } catch (exception &e) {
      LOG(WARNING) << e.what();
      // Ignore errors
    }
    try {
      // stats_["/total-bytes-write"] += reply.body().size();
      reply.Write(sock.get());
      sock->Flush();
      if (reply.IsSuccess())
        ++stats_["/request-success"];
      else
        ++stats_["/request-failure"];
    } catch (exception &e) {
      LOG(WARNING) << e.what();
      ++stats_["/request-failure"];
    }
  }
}

RequestHandler *HttpServer::request_handler() {
  return request_handler_.get();
}

thread *HttpServer::listen_thread() const {
  return listen_thread_.get();
}

void HttpServer::AddExportHandler() {
  request_handler()->AddPath("/export_vars$", &HttpServer::HandleExportVars, this);
}

bool HttpServer::HandleExportVars(const HttpRequest &request, HttpReply *reply) {
  reply->SetStatus(HttpReply::OK);
  reply->SetContentType("text/plain");
  string c;
  VariableExporter::GetGlobalExporter()->ExportToString(&c);
  reply->mutable_body()->CopyFrom(c);
  return true;
}

}  // namespace http
}  // namespace openinstrument
