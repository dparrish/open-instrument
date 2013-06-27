/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_INDEXED_STORE_FILE_H_
#define _OPENINSTRUMENT_LIB_INDEXED_STORE_FILE_H_

#include <list>
#include <string>
#include "lib/common.h"
#include "lib/file.h"
#include "lib/protobuf.h"
#include "lib/string.h"
#include "lib/timer.h"
#include "lib/trie.h"
#include "lib/variable.h"

namespace openinstrument {

class IndexedStoreFile : private noncopyable, public Refcountable {
 public:
  IndexedStoreFile(const string &filename);

  bool Open();
  void Close();
  void Clear();

  void RefcountDestroy() {
    Close();
    Clear();
    delete this;
  }

  bool GetVariable(const Variable &variable, vector<proto::ValueStream> *results);

  vector<proto::ValueStream> GetVariable(const Variable &variable) {
    vector<proto::ValueStream> results;
    GetVariable(variable, &results);
    return results;
  }

  const proto::StoreFileHeader &header() const {
    return header_;
  }

  string filename;

 private:
  scoped_ptr<ProtoStreamReader> reader_;
  bool opened_;
  unordered_map<string, proto::ValueStream> log_data_;
  proto::StoreFileHeader header_;
};

}  // namespace

#endif  // _OPENINSTRUMENT_LIB_INDEXED_STORE_FILE_H_
