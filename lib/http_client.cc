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
  sock.Connect(request.uri.hostname, request.uri.port, deadline);

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
        reply->SetStatus(static_cast<HttpReply::status_type>(lexical_cast<uint16_t>(ConsumeFirstWord(&output))));
      } catch (exception) {
        LOG(WARNING) << "Invalid HTTP response";
        reply->SetStatus(HttpReply::INTERNAL_SERVER_ERROR);
        return NULL;
      }
      break;
    }

    reply->ReadAndParseHeaders(&sock, deadline);

    if (reply->chunked_encoding()) {
      VLOG(2) << "Reading chunked encoding response";
      while (true) {
        try {
          string str;
          // Chunk header should be a hex length then \r\n
          sock.read_buffer()->ConsumeLine(&str);
          VLOG(2) << "  read length (" << str << ")";
          uint32_t len = 0;
          try {
            len = HexToUint32(str);
          } catch (std::out_of_range) {
            throw runtime_error(StringPrintf("Invalid chunk header reading HTTP response: %s", str.c_str()));
          }
          if (len == 0) {
            // Last chunk
            VLOG(2) << "  last chunk";
            sock.read_buffer()->ConsumeLine(&str);
            break;
          }
          // Body is terminated by a newline
          while (sock.read_buffer()->size() < len + 2) {
            try {
              sock.Read(deadline);
            } catch (exception) {
              throw runtime_error(StringPrintf("No response received from %s", sock.remote().ToString().c_str()));
            }
          }
          string tmp;
          sock.read_buffer()->Consume(len, &tmp);
          reply->mutable_body()->CopyFrom(tmp);
          VLOG(2) << "  got " << len << " bytes of body, total body size is now " << reply->body().size();
          // Throw away the next newline
          sock.read_buffer()->ConsumeLine(NULL);
        } catch (std::out_of_range) {
          sock.Read(deadline);
        }
      }
    } else if (reply->GetContentLength()) {
      VLOG(2) << "Reading response with content-length";
      while (sock.read_buffer()->size() < reply->GetContentLength()) {
        try {
          sock.Read(deadline);
        } catch (exception) {
          throw runtime_error(StringPrintf("No response received from %s", sock.remote().ToString().c_str()));
        }
      }
      reply->mutable_body()->CopyFrom(*(sock.read_buffer()));
    } else {
      VLOG(2) << "Reading entire response";
      try {
        while (sock.Read(deadline) > 0);
      } catch (exception) {
        throw runtime_error(StringPrintf("No response received from %s", sock.remote().ToString().c_str()));
      }
      reply->mutable_body()->CopyFrom(*(sock.read_buffer()));
    }
  } catch (exception& e) {
    LOG(ERROR) << "Exception: " << e.what() << "\n";
  }
  reply->mutable_headers()->SetHeader("Content-Length", reply->body().size());

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
