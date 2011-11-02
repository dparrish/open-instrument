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
#include "lib/trie.h"

namespace openinstrument {

class IndexedStoreFile : private noncopyable {
 public:
  typedef Trie<proto::ValueStream> MapType;

  explicit IndexedStoreFile(const string &filename)
    : filename_(filename),
      fh_(filename) {}

  ~IndexedStoreFile() {
    Clear();
  }

  static proto::ValueStream &LoadAndGetVar(const Variable &variable, uint64_t timestamp);
  void Clear();
  bool ReadHeader();
  proto::ValueStream &LoadVariable(const Variable &variable);

  // Return a list of variables contained in this file that match the provided search variable.
  // Supports label searches
  list<Variable> ListVariables(const Variable &variable);

  inline bool ContainsVariable(const Variable &variable) {
    return log_data_.find(variable.ToString()) != log_data_.end();
  }

 private:
  string filename_;
  File fh_;
  MapType log_data_;
  proto::StoreFileHeader file_header_;
};


}  // namespace

#endif  // _OPENINSTRUMENT_LIB_DATASTORE_H_
