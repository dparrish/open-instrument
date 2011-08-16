/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef OPENINSTRUMENT_LIB_MIME_TYPES_H_
#define OPENINSTRUMENT_LIB_MIME_TYPES_H_

#include <stdio.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

namespace openinstrument {

class MimeTypes : private noncopyable {
 public:
  const string &Lookup(const string &filename);
  uint32_t ReadFile(const string &filename);

 private:
  unordered_map<string, string> types_;
};

}  // namespace openinstrument

#endif  // OPENINSTRUMENT_LIB_MIME_TYPES_H_
