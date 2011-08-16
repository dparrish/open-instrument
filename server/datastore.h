/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_DATASTORE_H_
#define _OPENINSTRUMENT_LIB_DATASTORE_H_

#include <list>
#include <string>
#include "lib/common.h"
#include "lib/file.h"
#include "lib/string.h"
#include "lib/timer.h"
#include "lib/protobuf.h"

namespace openinstrument {

class IndexedStoreFile : private noncopyable {
 public:
  typedef unordered_map<string, proto::ValueStream> MapType;

  explicit IndexedStoreFile(const string &filename)
    : filename_(filename),
      fh_(filename) {}

  ~IndexedStoreFile() {
    Clear();
  }

  static proto::ValueStream &LoadAndGetVar(const string &variable, uint64_t timestamp);
  void Clear();
  bool ReadHeader();
  proto::ValueStream &LoadVariable(const string &variable);

  // Return a list of variables contained in this file that match the provided search variable.
  // Supports label searches
  list<Variable> ListVariables(const string &variable);

  inline bool ContainsVariable(const string &variable) {
    return log_data_.find(variable) != log_data_.end();
  }

 private:
  string filename_;
  LocalFile fh_;
  MapType log_data_;
  proto::StoreFileHeader file_header_;
};


}  // namespace

#endif  // _OPENINSTRUMENT_LIB_DATASTORE_H_
