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
#include "lib/socket.h"
#include "lib/string.h"

namespace openinstrument {
namespace http {

const char *HttpMessage::header_sep_ = ": ";
const char *HttpMessage::crlf_ = "\r\n";

void HttpMessage::WriteHeader(Socket *sock) {
  if (!status_written_)
    WriteFirstline(sock);
  if (!header_written_) {
    for (size_t i = 0; i < headers_.size(); ++i) {
      if (chunked_encoding() && headers_[i].name == "Content-Length")
        continue;
      sock->Write(headers_[i].name);
      sock->Write(header_sep_);
      sock->Write(headers_[i].value);
      sock->Write(crlf_);
      VLOG(2) << "        " << headers_[i].name << ": " << headers_[i].value;
    }
    sock->Write(crlf_);
  }
  status_written_ = true;
  header_written_ = true;
}

void HttpMessage::set_http_version(const string &version) {
  boost::regex regex("(?i)HTTP/(\\d+)\\.(\\d+)");
  boost::smatch what;
  if (!boost::regex_match(version, what, regex) || what.size() != 3) {
    version_major_ = version_minor_ = 0;
  } else {
    version_major_ = atoi(string(what[1]).c_str());
    version_minor_ = atoi(string(what[2]).c_str());
  }
}

string HttpMessage::http_version() const {
  return StringPrintf("HTTP/%u.%u", version_major_, version_minor_);
}

void HttpMessage::SetHeader(const char *key, const char *value) {
  headers_.SetHeader(key, value);
}

const HttpHeaders &HttpMessage::headers() const {
  return headers_;
}

void HttpMessage::SetContentLength(size_t length) {
  if (length == 0)
    length = body_.size();
  headers_.SetHeader("Content-Length", lexical_cast<string>(length));
}

uint64_t HttpMessage::GetContentLength() const {
  const string &header = headers_.GetHeader("Content-Length");
  if (!header.size())
    return 0;
  return lexical_cast<uint64_t>(header);
}

void HttpMessage::SetContentType(const char *type) {
  headers_.SetHeader("Content-Type", type);
}

const string &HttpMessage::GetContentType() {
  static string unknown_type = "application/unknown";
  const string &header = headers_.GetHeader("Content-Type");
  if (!header.size())
    return unknown_type;
  return header;
}

void HttpMessage::Write(Socket *sock) {
  WriteHeader(sock);
  if (chunked_encoding_) {
    BOOST_FOREACH(CordBuffer &buffer, body_) {
      WriteChunk(sock, StringPiece(buffer.buffer(), buffer.size()));
    }
    WriteLastChunk(sock);
  } else {
    if (body_.size())
      sock->Write(body_);
  }
}

void HttpMessage::WriteChunk(Socket *sock, const StringPiece &chunk) {
  if (chunk.size() == 0)
    return;
  sock->Write(HexToBuffer(chunk.size()));
  sock->Write("\r\n");
  sock->Write(chunk);
  sock->Write("\r\n");
}

void HttpMessage::WriteLastChunk(Socket *sock) {
  sock->Write("0\r\n\r\n");
}

void HttpMessage::ReadAndParseHeaders(Socket *sock, Deadline deadline) {
  for (size_t i = 0; i < 5000; ++i) {
    string output;
    try {
      sock->read_buffer()->ConsumeLine(&output);
    } catch (out_of_range) {
      try {
        sock->Read(deadline);
        continue;
      } catch (exception) {
        throw runtime_error(StringPrintf("No response received from %s", sock->remote().ToString().c_str()));
      }
    }

    // End of headers
    if (output.empty())
      break;

    if (output.at(0) == ' ' || output.at(0) == '\t') {
      // continuation of last output
      headers_.AppendLastHeader(StringTrim(output));
    } else {
      // new output
      string::size_type pos = output.find(":");
      if (pos == string::npos) {
        LOG(WARNING) << "Invalid output line: " << output;
        continue;
      }
      headers_.AddHeader(output.substr(0, pos), StringTrim(output.substr(pos + 1)));
    }
  }

  if (headers_.GetHeader("Transfer-Encoding").find("chunked") != string::npos) {
    set_chunked_encoding(true);
  } else {
    set_chunked_encoding(false);
  }
}

}  // namespace http
}  // namespace openinstrument
