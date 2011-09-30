/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef OPENINSTRUMENT_LIB_HTTP_HTTPHEADERS_H_
#define OPENINSTRUMENT_LIB_HTTP_HTTPHEADERS_H_

#include <string>
#include <vector>
#include "lib/common.h"

namespace openinstrument {
namespace http {

class HttpHeader {
 public:
  HttpHeader(const string &name, const string &value)
    : name(name),
      value(value) {}
  string name;
  string value;
};

class HttpHeaders {
 public:
  // Sets the value of a header "name" to "value". If the header already exists, its value is replaced.
  void SetHeader(const string &name, const string &value) {
    for (size_t i = 0; i < headers_.size(); ++i) {
      if (headers_[i].name == name) {
        headers_[i].value = value;
        return;
      }
    }
    AddHeader(name, value);
  }

  void SetHeader(const string &name, uint64_t value) {
    SetHeader(name, lexical_cast<string>(value));
  }

  // Sets the value of a header "name" to "value". If the header already
  // exists, another header will be added with the new value.
  void AddHeader(const string &name, const string &value) {
    headers_.push_back(HttpHeader(name, value));
  }

  void AddHeader(const string &name, uint64_t value) {
    AddHeader(name, lexical_cast<string>(value));
  }

  void AppendLastHeader(const string append) {
    HttpHeader last = headers_.back();
    headers_.pop_back();
    last.value += append;
    headers_.push_back(last);
  }

  // Checks whether the header is set at all. Does not check how many instances of a header are set.
  bool HeaderExists(const string &name) const {
    for (size_t i = 0; i < headers_.size(); ++i) {
      if (headers_[i].name == name)
        return true;
    }
    return false;
  }

  void RemoveHeader(const string &name) {
    for (vector<HttpHeader>::iterator i = headers_.begin(); i != headers_.end(); ++i) {
      if (i->name == name) {
        headers_.erase(i);
        i = headers_.begin();
        continue;
      }
    }
  }

  // Gets the first value of a header from the set.
  const string &GetHeader(const string &name) const {
    static string emptystring;
    for (size_t i = 0; i < headers_.size(); ++i) {
      if (headers_[i].name == name)
        return headers_[i].value;
    }
    return emptystring;
  }

  // Gets every value of a set header.
  vector<string> GetHeaderValues(const string &name) const {
    vector<string> values;
    for (size_t i = 0; i < headers_.size(); ++i) {
      if (headers_[i].name == name)
        values.push_back(headers_[i].value);
    }
    return values;
  }

  bool empty() const {
    return headers_.empty();
  }

  size_t size() const {
    return headers_.size();
  }

  const HttpHeader &back() const {
    return headers_.back();
  }

  const HttpHeader &operator[](int index) const {
    return headers_[index];
  }

 private:
  vector<HttpHeader> headers_;
};

}  // namespace http
}  // namespace openinstrument

#endif  // OPENINSTRUMENT_LIB_HTTP_HTTPHEADERS_H_
