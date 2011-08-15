/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_URI_
#define _OPENINSTRUMENT_LIB_URI_

#include <string>
#include <vector>
#include "lib/common.h"

namespace openinstrument {
namespace http {

using std::string;

class Uri {
 public:
  Uri() : protocol("http"), port(80) {}

  explicit Uri(const string &in) : protocol("http"), port(80) {
    ParseUri(in);
  }

  struct RequestParam {
    string key;
    string value;
  };

  bool ParseUri(const string &in);
  std::vector<RequestParam> ParseQueryString(const string &in);
  static bool UrlEncode(const string &in, string &out);
  static string UrlEncode(const string &in);
  static bool UrlDecode(const string &in, string &out);
  static string UrlDecode(const string &in);
  string Assemble() const;
  void SetParam(const string &key, const string &value);
  void AddParam(const string &key, const string &value);
  const string &GetParam(const string &key) const;

  inline void set_protocol(const string &newval) {
    protocol = newval;
  }

  inline void set_username(const string &newval) {
    username = newval;
  }

  inline void set_password(const string &newval) {
    password = newval;
  }

  inline void set_port(uint16_t newval) {
    port = newval;
  }

  inline void set_hostname(const string &newval) {
    hostname = newval;
  }

  inline void set_path(const string &newval) {
    path = newval;
  }

  inline void set_query_string(const string &newval) {
    query_string = newval;
  }

  string protocol;
  string username;
  string password;
  string hostname;
  uint16_t port;
  string path;
  string query_string;
  vector<RequestParam> params;
};

}  // namespace http
}  // namepsace openinstrument

#endif  // _OPENINSTRUMENT_LIB_URI_
