/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_STORE_FILE_MANAGER_H_
#define _OPENINSTRUMENT_LIB_STORE_FILE_MANAGER_H_

#include <vector>
#include <set>
#include <string>
#include "lib/common.h"
#include "lib/file.h"
#include "lib/string.h"
#include "lib/timer.h"
#include "server/indexed_store_file.h"

DECLARE_string(datastore);

namespace openinstrument {

class StoreFileManager {
 public:
  StoreFileManager() {
    ReglobFiles();
  }

  ~StoreFileManager() {
    for (auto i : store_files_) {
      CloseFile(i.second->filename);
    }
  }

  void ReglobFiles() {
    store_filenames_.clear();
    store_filenames_ = Glob(StringPrintf("%s/datastore.*.bin", FLAGS_datastore.c_str()));
    VLOG(2) << "Store filenames:";
    for (auto filename : store_filenames_) {
      VLOG(2) << "  " << filename;
      auto header = OpenFile(filename);
      if (!header) {
        CloseFile(filename);
        continue;
      }
    }
  }

  const vector<string> &available_files() const {
    return store_filenames_;
  }

  int16_t num_open_files() const {
    return store_files_.size();
  }

  const proto::StoreFileHeader *OpenFile(const string &filename) {
    scoped_ptr<IndexedStoreFile> file(new IndexedStoreFile(filename));
    if (!file.get())
      return NULL;
    store_files_[filename] = file.release();
    return &store_files_[filename]->header;
  }

  void CloseFile(const string &filename) {
    auto i = store_files_.find(filename);
    if (i == store_files_.end())
      return;
    if (i->second->filename == filename) {
      store_files_.erase(i);
      delete i->second;
      return;
    }
  }

  const proto::StoreFileHeader *GetHeader(const string &filename) {
    auto i = store_files_.find(filename);
    if (i == store_files_.end())
      return NULL;
    return &i->second->header;
  }

 private:
  vector<string> store_filenames_;
  unordered_map<string, IndexedStoreFile *> store_files_;

};

}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_STORE_FILE_MANAGER_H_
