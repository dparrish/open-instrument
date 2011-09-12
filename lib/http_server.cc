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

HttpServer::~HttpServer() {
  Stop();
}

void HttpServer::Start() {
  listen_socket_.Listen(address_);
  // Block all signals
  sigset_t new_mask;
  sigfillset(&new_mask);
  sigset_t old_mask;
  pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

  LOG(INFO) << "HttpServer listening on " << address_.ToString();

  // Create a pool of threads to run all of the io_services.
  vector<shared_ptr<thread> > threads;
  for (size_t i = 0; i < thread_pool_size_; ++i) {
    threads.push_back(shared_ptr<thread>(new thread(bind(&HttpServer::Acceptor, this))));
  }

  stats_["/worker-threads-total"] = thread_pool_size_;
  stats_["/worker-threads-busy"] = 0;

  // Wait for all threads in the pool to exit.
  for (size_t i = 0; i < threads.size(); ++i)
    threads[i]->join();
}

void HttpServer::Stop() {
  LOG(INFO) << "Stopping HttpServer";
  shutdown_ = true;
  if (listen_thread_.get())
    listen_thread_->join();
}

void HttpServer::Acceptor() {
  while (true) {
    if (shutdown_)
      break;
    scoped_ptr<Socket> client(listen_socket_.Accept(1000));
    if (!client.get())
      continue;
    ++stats_["/connections-received"];
    ++stats_["/worker-threads-busy"];
    VLOG(1) << "Accepted new connection from " << client->remote().ToString();
    try {
      HandleClient(client.get());
      client->Flush();
    } catch (exception &e) {
      LOG(WARNING) << e.what();
    }
    --stats_["/connections-received"];
    --stats_["/worker-threads-busy"];
  }
}

void HttpServer::HandleClient(Socket *sock) {
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
          throw runtime_error(StringPrintf("No response received from %s", sock->remote().ToString().c_str()));
        }
      }

      try {
        request.set_method(ConsumeFirstWord(&output));
        request.uri = Uri(ConsumeFirstWord(&output));
        request.set_http_version(ConsumeFirstWord(&output));
        if (request.http_version() == "HTTP/0.0")
          throw runtime_error("Invalid HTTP version");
        request.ReadAndParseHeaders(sock, deadline);
      } catch (exception &e) {
        LOG(WARNING) << "Invalid HTTP request: " << e.what();
        HttpReply reply;
        reply.StockReply(HttpReply::BAD_REQUEST);
        reply.Write(sock);
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
          throw runtime_error(StringPrintf("No POST received from %s", sock->remote().ToString().c_str()));
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

    request_handler_->HandleRequest(request, &reply);

    try {
      // Set default required headers in the reply
      if (!reply.headers().HeaderExists("Content-Length"))
        reply.SetContentLength(reply.body().size());
      if (!reply.headers().HeaderExists("Content-Type"))
        reply.SetContentType("text/html; charset=UTF-8");
      if (!reply.headers().HeaderExists("Date"))
        reply.mutable_headers()->AddHeader("Date", Timestamp().GmTime(RFC112Format));
      if (!reply.headers().HeaderExists("Last-Modified"))
        reply.mutable_headers()->AddHeader("Last-Modified", Timestamp().GmTime(RFC112Format));
      if (!reply.headers().HeaderExists("Server"))
        reply.mutable_headers()->AddHeader("Server", "OpenInstrument/1.0");
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
      reply.Write(sock);
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

HttpServer::HttpServer *HttpServer::NewServer(const string &addr, uint16_t port, size_t num_threads) {
  try {
    // Run server in background thread.
    HttpServer *server = new http::HttpServer(addr, port, num_threads);
    server->listen_thread_.reset(new thread(bind(&HttpServer::Start, server)));
    return server;
  } catch (exception &e) {
    LOG(ERROR) << "exception: " << e.what();
    return NULL;
  }
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
