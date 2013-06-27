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
#include <string>
#include "lib/common.h"
#include "lib/string.h"

namespace proto { class StoreFileHeader; }

namespace openinstrument {

class RetentionPolicyManager;
class IndexedStoreFile;

class StoreFileManager : private noncopyable {
 public:
  StoreFileManager(RetentionPolicyManager &retention_policy_manager);
  ~StoreFileManager();
  vector<string> available_files() const;
  int16_t num_open_files() const;
  const proto::StoreFileHeader &OpenFile(const string &filename);
  const proto::StoreFileHeader &GetHeader(const string &filename);
  void CloseFile(const string &filename);

 private:
  void RunRetentionPolicy();
  void BackgroundThread();
  void WatchChanged(const string &path, const string &filename);
  void ReglobFiles();

  bool shutdown_;
  thread background_thread_;
  unordered_map<string, IndexedStoreFile *> store_files_;
  RetentionPolicyManager &retention_policy_manager_;

  mutable Mutex store_files_mutex_;
};

}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_STORE_FILE_MANAGER_H_
