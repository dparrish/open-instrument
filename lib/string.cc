/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include "lib/string.h"

namespace openinstrument {

using std::string;

const char *StringPiece::c_str() {
  if (!size_)
    return NULL;
  if (!str_.get())
    str_.reset(new string(ptr_, size_));
  return str_->c_str();
}

char StringPiece::at(uint64_t index) const {
  if (index >= size_)
    throw out_of_range(StringPrintf("Index %llu too high", index));
  return *(data() + index);
}

StringPiece StringPiece::substr(uint64_t start, int64_t size) const {
  if (start >= size_)
    throw out_of_range(StringPrintf("Start %llu too high", start));
  if (size > 0 && start + size >= size_)
    throw out_of_range("Not enough bytes in StringPiece");
  if (size)
    return StringPiece(data() + start, size);
  else
    return StringPiece(data() + start, size_ - start);
}

void StringAppendf(string *str, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  char *buf = new char[4096];
  while (1) {
    va_list backupap;
    va_copy(backupap, ap);
    int n = vsnprintf(buf, 4095, format, backupap);
    va_end(backupap);
    if (n > 4095) {
      delete[] buf;
      buf = new char[n + 1];
    } else {
      str->append(buf);
      delete[] buf;
      return;
    }
  }
}

string StringPrintf(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  char *buf = new char[4096];
  while (1) {
    va_list backupap;
    va_copy(backupap, ap);
    int n = vsnprintf(buf, 4095, format, backupap);
    va_end(backupap);
    if (n > 4095) {
      delete[] buf;
      buf = new char[n + 1];
    } else {
      string str(buf);
      delete[] buf;
      return str;
    }
  }
}

string StringTrim(const string &str) {
  string::size_type pos1 = str.find_first_not_of(' ');
  string::size_type pos2 = str.find_last_not_of(' ');
  return str.substr(pos1 == string::npos ? 0 : pos1, pos2 == string::npos ? str.length() - 1 : pos2 - pos1 + 1);
}

string ConsumeFirstWord(string *input) {
  size_t pos = input->find(' ');
  if (pos == string::npos) {
    // No spaces, return the whole string
    string ret = *input;
    input->clear();
    return ret;
  }
  string ret;
  try {
    ret = input->substr(0, pos);
    input->erase(0, pos + 1);
  } catch (exception) {
    input->clear();
  }
  return ret;
}


}  // namespace
