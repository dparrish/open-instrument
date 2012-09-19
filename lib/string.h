/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_STRING_
#define _OPENINSTRUMENT_LIB_STRING_

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include "lib/common.h"

namespace openinstrument {

using std::string;

class StringPiece {
 public:
  explicit StringPiece(const string &str) : ptr_(str.data()), size_(str.size()), str_(NULL) {}
  StringPiece(const char *ptr, uint64_t size) : ptr_(ptr), size_(size), str_(NULL) {}
  StringPiece(const StringPiece &a) : ptr_(a.ptr_), size_(a.size_), str_(NULL) {}

  string ToString() const {
    return string(ptr_, size_);
  }

  void AppendToString(string *output) const {
    output->append(ptr_, size_);
  }

  void Reset(const char *ptr, uint64_t size) {
    CHECK(ptr);
    ptr_ = ptr;
    size_ = size;
  }

  // Returns a c-style string.
  // WARNING: This makes a copy of the data pointed to by this object, as there needs to be extra space for a trailing
  // null.
  const char *c_str();

  const char *data() const {
    return ptr_;
  }

  uint64_t size() const {
    return ptr_ ? size_ : 0;
  }

  bool operator==(const StringPiece &a) const {
    if (a.size() != size())
      return false;
    return strncmp(a.data(), data(), size()) == 0;
  }

  bool operator!=(const StringPiece &a) const {
    return !operator==(a);
  }

  operator bool() const {
    return size_ > 0;
  }

  char operator[](uint64_t index) const {
    return at(index);
  }

  char at(uint64_t index) const;
  StringPiece substr(uint64_t start, int64_t size = -1) const;

 private:
  const char *ptr_;
  uint64_t size_;
  boost::scoped_ptr<string> str_;
};

void StringAppendf(string *str, const char *format, ...);
string StringPrintf(const char *format, ...);
string StringTrim(const string &str);

// Consume and return the first word in the supplied string.
string ConsumeFirstWord(string *input);

inline char HexToChar(const string &in) {
  char out;
  if (sscanf(in.c_str(), "%02hhx", &out) != 1)
    throw std::out_of_range("Invalid hex character");
  return out;
}

inline uint16_t HexToUint16(const string &in) {
  uint16_t out;
  if (sscanf(in.c_str(), "%04hx", &out) != 1)
    throw std::out_of_range("Invalid hex character");
  return out;
}

inline uint32_t HexToUint32(const string &in) {
  unsigned long out;
  if (sscanf(in.c_str(), "%08lx", &out) != 1)
    throw std::out_of_range("Invalid hex character");
  return out;
}

inline uint64_t HexToUint64(const string &in) {
  unsigned long long out;
  if (sscanf(in.c_str(), "%02llx", &out) != 1)
    throw std::out_of_range("Invalid hex uint64");
  return out;
}

const string SiUnits(uint64_t value, uint8_t precision=0, const char *suffix="b");


}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_STRING_
