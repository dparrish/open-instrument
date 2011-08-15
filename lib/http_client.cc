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
#include "lib/http_client.h"

namespace openinstrument {
namespace http {

HttpReply *HttpClient::SendRequest(HttpRequest &request) {
  scoped_ptr<HttpReply> reply(new HttpReply());
  VLOG(1) << "Requesting " << request.uri.Assemble();
  Deadline deadline(deadline_time_);
  Socket sock;
  try {
    sock.Connect(request.uri.hostname, request.uri.port, deadline);
  } catch (exception& e) {
    LOG(ERROR) << "Couldn't connect to " << request.uri.hostname << ":" << request.uri.port << ": " << e.what() << "\n";
  }

  try {
    // Send request
    request.Write(&sock);

    // Wait for response
    Cord response;
    string output;
    for (int i = 0; i < 20; ++i) {
      try {
        output.clear();
        sock.read_buffer()->ConsumeLine(&output);
      } catch (out_of_range) {
        try {
          sock.Read(deadline);
          continue;
        } catch (exception) {
          throw runtime_error(StringPrintf("No response received from %s", sock.remote().ToString().c_str()));
        }
      }

      try {
        reply->set_http_version(ConsumeFirstWord(&output));
        reply->set_status(static_cast<HttpReply::status_type>(lexical_cast<uint16_t>(ConsumeFirstWord(&output))));
      } catch (exception) {
        LOG(WARNING) << "Invalid HTTP response";
        reply->set_status(HttpReply::INTERNAL_SERVER_ERROR);
        return NULL;
      }
      break;
    }

    reply->ReadAndParseHeaders(&sock, deadline);

    if (reply->GetContentLength()) {
      while (sock.read_buffer()->size() < reply->GetContentLength()) {
        try {
          sock.Read(deadline);
        } catch (exception) {
          throw runtime_error(StringPrintf("No response received from %s", sock.remote().ToString().c_str()));
        }
      }
    } else {
      try {
        while (sock.Read(deadline) > 0);
      } catch (exception) {
        throw runtime_error(StringPrintf("No response received from %s", sock.remote().ToString().c_str()));
      }
      reply->mutable_headers()->SetHeader("Content-Length", sock.read_buffer()->size());
    }
    reply->mutable_body()->CopyFrom(*(sock.read_buffer()));
  } catch (exception& e) {
    LOG(ERROR) << "Exception: " << e.what() << "\n";
  }

  return reply.release();
}

HttpRequest *HttpClient::NewRequest(const string &url) {
  HttpRequest *request = new HttpRequest();
  request->uri.ParseUri(url);
  request->set_http_version("HTTP/1.1");
  request->set_method("GET");
  request->SetHeader("Host", request->uri.hostname.c_str());
  request->SetHeader("Accept", "*/*");
  request->SetHeader("Connection", "close");
  return request;
}

}  // namespace http
}  // namespace openinstrument
