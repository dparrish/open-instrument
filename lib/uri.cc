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
#include "lib/string.h"
#include "lib/uri.h"

namespace openinstrument {
namespace http {

namespace {
// Pre-compile the regular expression
boost::regex url_regex("^(?:(\\w+)://)?"  // protocol
                       "(?:([^@:]+)(?::([^@]+))?@)?"  // username & password
                       "([^/]+?)?"        // hostname
                       "(?::(\\d+))?"
                       "(/[^\\?\\&]*)?"   // path
                       "(?:\\?(.*))?");   // query string;
}

bool Uri::ParseUri(const string &in) {
  boost::cmatch what;
  VLOG(1) << "Parsing URL " << in;
  if (!regex_match(in.c_str(), what, url_regex)) {
    LOG(INFO) << "Unparseable URL " << in;
    return false;
  }

  if (what[1].first != what[1].second)
    protocol = string(what[1].first, what[1].second);
  if (what[2].first != what[2].second)
    username = UrlDecode(string(what[2].first, what[2].second));
  if (what[3].first != what[3].second)
    password = UrlDecode(string(what[3].first, what[3].second));
  if (what[4].first != what[4].second)
    hostname = string(what[4].first, what[4].second);
  if (what[5].first != what[5].second)
    port = atoi(string(what[5].first, what[5].second).c_str());
  if (what[6].first != what[6].second)
    path = UrlDecode(string(what[6].first, what[6].second));
  if (what[7].first != what[7].second)
    query_string = string(what[7].first, what[7].second);

  if (query_string.size())
    params = ParseQueryString(query_string);
  return true;
}

vector<Uri::RequestParam> Uri::ParseQueryString(const string &in) {
  vector<Uri::RequestParam> params;
  int state = 0;
  RequestParam param;
  for (size_t i = 0; i < in.size(); ++i) {
    switch (state) {
      case 0 :  // Reading key
        if (in[i] == '=') {
          // End of key
          if (!param.key.size()) {
            LOG(WARNING) << "Empty key parsing query string";
            return params;
          }
          state = 1;
        } else {
          param.key += in[i];
        }
        break;
      case 1:  // reading value
        if (in[i] == '&') {
          // End of value
          param.key = UrlDecode(param.key);
          param.value = UrlDecode(param.value);
          params.push_back(param);
          param.key = "";
          param.value = "";
          state = 0;
        } else {
          param.value += in[i];
        }
        break;
    }
  }
  if (param.key.size()) {
    param.key = UrlDecode(param.key);
    param.value = UrlDecode(param.value);
    params.push_back(param);
  }
  return params;
}

bool Uri::UrlEncode(const string &in, string &out) {
  out.clear();
  // Output string could potentially contain 3x the amount of characters as the input
  out.reserve(in.size() * 3);
  for (size_t i = 0; i < in.size(); ++i) {
    if ((in[i] >= 'a' && in[i] <= 'z') || (in[i] >= 'A' && in[i] <= 'Z') ||
        (in[i] >= '0' && in[i] <= '9') ||
        in[i] == '/') {
      // Normal characters are simply appended
      out += in[i];
      continue;
    }

    // Anything else is urlencoded
    out += openinstrument::StringPrintf("%%%02x", in[i]);
  }
  return true;
}

string Uri::UrlEncode(const string &in) {
  string out;
  if (UrlEncode(in, out))
    return out;
  return "";
}

bool Uri::UrlDecode(const string &in, string &out) {
  out.clear();
  out.reserve(in.size());
  for (size_t i = 0; i < in.size(); ++i) {
    if (in[i] == '%') {
      // Check that there's at least 2 more characters after the %
      if (i + 3 > in.size()) {
        LOG(WARNING) << "Input string does not contain 2 characters after %";
        return false;
      }

      // Read the next 2 characters as a hexadecimal number and convert it
      string ch = in.substr(i + 1, 2);
      unsigned char value = HexToChar(ch);
      VLOG(3) << "Got hex character " << ch << " converted to " << value;
      out += value;
      i += 2;
    } else if (in[i] == '+') {
      out += ' ';
    } else {
      out += in[i];
    }
  }
  return true;
}

string Uri::UrlDecode(const string &in) {
  string out;
  if (UrlDecode(in, out))
    return out;
  return "";
}

string Uri::Assemble() const {
  string url;
  if (protocol.size())
    url += protocol + "://";
  else
    url += "http://";
  if (username.size()) {
    url += UrlEncode(username);
    if (password.size())
      url += ":" + UrlEncode(password);
    url += "@";
  }
  if (hostname.size())
    url += hostname;
  if (port && port != 80)
    url += ":" + lexical_cast<string>(port);
  if (path.size()) {
    if (path[0] != '/')
      url += "/";
    url += path;
  } else {
    url += "/";
  }
  if (params.size()) {
    url += "?";
    for (size_t i = 0; i < params.size(); ++i) {
      if (i > 0)
        url += "&";
      url += UrlEncode(params[i].key) + "=" + UrlEncode(params[i].value);
    }
  }
  return url;
}

void Uri::SetParam(const string &key, const string &value) {
  for (auto &i : params) {
    if (i.key == key) {
      i.value = value;
      return;
    }
  }
  AddParam(key, value);
}

void Uri::AddParam(const string &key, const string &value) {
  RequestParam param;
  param.key = key;
  param.value = value;
  params.push_back(param);
}

const string &Uri::GetParam(const string &key) const {
  static const string empty_string("");
  for (vector<Uri::RequestParam>::const_iterator i = params.begin(); i != params.end(); ++i) {
    if (i->key == key)
      return i->value;
  }
  return empty_string;
}

}  // namespace http
}  // namepsace openinstrument
