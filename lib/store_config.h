/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef OPENINSTRUMENT_SERVER_STORE_CONFIG_H_
#define OPENINSTRUMENT_SERVER_STORE_CONFIG_H_

#include <string>
#include <google/protobuf/text_format.h>
#include "lib/common.h"
#include "lib/exported_vars.h"
#include "lib/file.h"
#include "lib/hash.h"
#include "lib/openinstrument.pb.h"
#include "lib/protobuf.h"
#include "lib/store_client.h"

namespace openinstrument {

class StoreConfig : private noncopyable {
 public:
  typedef HashRing<string, string> HashRingType;

  static proto::StoreConfig &get();
  static StoreConfig &get_manager();
  ~StoreConfig();

  void SetConfigFilename(const string &filename);
  void AddReloadCallback(const Callback &callback);
  void RunCallbacks();
  void set_config(const proto::StoreConfig &config);
  void HandleNewConfig(const proto::StoreConfig &config);
  bool ReadConfig(const string &input);
  void UpdateHashRing();
  const HashRingType &hash_ring() const;
  proto::StoreServer::State GetServerState(const string &address) const;
  void SetServerState(const string &address, proto::StoreServer::State state);
  proto::StoreServer *server(const string &address);
  bool ReadConfigFile();
  void WaitForConfigLoad();
  void WriteConfigFile();
  string DumpConfig() const;
  proto::StoreConfig &config();

 private:
  StoreConfig(const string &filename);
  void ConfigThread();
  void KillConfigThread();

  proto::StoreConfig config_;
  string config_file_;
  mutable Mutex mutex_;
  scoped_ptr<thread> config_thread_;
  bool shutdown_;
  vector<Callback> reload_callbacks_;
  Notification initial_load_notify_;
  FileStat config_stat_;
  string my_address_;
  HashRingType ring_;

  static StoreConfig *global_config_;
  static Mutex global_config_mutex_;
};

}  // namespace openinstrument

#endif  // OPENINSTRUMENT_SERVER_STORE_CONFIG_H_
