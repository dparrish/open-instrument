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
#include "lib/protobuf.h"
#include "lib/string.h"
#include "lib/timer.h"
#include "lib/trie.h"
#include "lib/variable.h"

namespace openinstrument {

class IndexedStoreFile : private noncopyable {
 public:
  IndexedStoreFile(const string &filename);
  vector<proto::ValueStream> GetVariable(const Variable &variable);
  bool GetVariable(const Variable &variable, vector<proto::ValueStream> *results);

  string filename;
  ProtoStreamReader reader;
  typedef unordered_map<string, proto::ValueStream> MapType;
  MapType log_data;
  proto::StoreFileHeader header;
};

}  // namespace

#endif  // _OPENINSTRUMENT_LIB_DATASTORE_H_
