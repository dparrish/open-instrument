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

  StoreConfig() : config_thread_(NULL), shutdown_(false), config_stat_("/dev/null"), ring_(2) {}

  explicit StoreConfig(const string &config_file)
    : config_file_(config_file),
      config_thread_(NULL),
      shutdown_(false),
      config_stat_("/dev/null"),
      ring_(2) {
    ReadConfigFile();
    WaitForConfigLoad();
    config_thread_.reset(new thread(bind(&StoreConfig::ConfigThread, this)));
  }

  ~StoreConfig() {
    KillConfigThread();
  }

  void AddReloadCallback(const Callback &callback) {
    reload_callbacks_.push_back(callback);
  }

  void RunCallbacks() {
    for (vector<Callback>::const_iterator i = reload_callbacks_.begin(); i != reload_callbacks_.end(); ++i) {
      (*i)();
    }
  }

  void set_config(const proto::StoreConfig &config) {
    config_.CopyFrom(config);
  }

  void HandleNewConfig(const proto::StoreConfig &config) {
    VLOG(1) << "Loaded new configuration with changes";
    MutexLock lock(mutex_);
    config_.CopyFrom(config);
    UpdateHashRing();
    RunCallbacks();
  }

  bool ReadConfig(const string &input) {
    proto::StoreConfig new_config;
    if (google::protobuf::TextFormat::MergeFromString(input, &new_config)) {
      HandleNewConfig(new_config);
      return true;
    }
    LOG(WARNING) << "Error reading new config";
    return false;
  }

  void UpdateHashRing() {
    ring_.Clear();
    for (int i = 0 ; i < config_.server_size(); i++) {
      const proto::StoreServer &server = config_.server(i);
      ring_.AddNode(server.address());
    }
  }

  const HashRingType &hash_ring() const {
    return ring_;
  }

  proto::StoreServer::State GetServerState(const string &address) const {
    MutexLock lock(mutex_);
    for (int i = 0 ; i < config_.server_size(); i++) {
      const proto::StoreServer &server = config_.server(i);
      if (server.address() == address) {
        return server.state();
      }
    }
    return proto::StoreServer::UNKNOWN;
  }

  void SetServerState(const string &address, proto::StoreServer::State state) {
    VLOG(3) << "Setting state for " << address << " to " << state;
    mutex_.lock();
    for (int i = 0 ; i < config_.server_size(); i++) {
      proto::StoreServer *server = config_.mutable_server(i);
      if (server->address() == address) {
        server->set_state(state);
        server->set_last_updated(Timestamp::Now());
        mutex_.unlock();
        WriteConfigFile();
        return;
      }
    }
    mutex_.unlock();
    LOG(INFO) << "Done setting server state";
  }

  proto::StoreServer *server(const string &address) {
    for (int i = 0 ; i < config_.server_size(); i++) {
      proto::StoreServer *server = config_.mutable_server(i);
      if (server->address() == address)
        return server;
    }
    return NULL;
  }

  bool ReadConfigFile() {
    if (config_file_.empty())
      return false;
    File fh(config_file_);
    FileStat stat = fh.stat();
    if (stat.size() < 5) {
      LOG(WARNING) << "Empty configuration file, not reading";
      return false;
    } else {
      string input;
      input.resize(stat.size());
      fh.Read(const_cast<char *>(input.data()), stat.size());
      initial_load_notify_.Notify();
      return ReadConfig(input);
    }
  }

  void WaitForConfigLoad() {
    if (config_file_.empty())
      return;
    initial_load_notify_.WaitForNotification();
  }

  void WriteConfigFile() {
    if (config_file_.empty())
      return;
    File fh(config_file_, "w");
    string output = DumpConfig();
    fh.Write(output);
    fh.Close();
    config_stat_ = FileStat(config_file_);
  }

  string DumpConfig() const {
    string output;
    MutexLock lock(mutex_);
    google::protobuf::TextFormat::PrintToString(config_, &output);
    return output;
  }

  const proto::StoreConfig &config() const {
    return config_;
  }

 private:
  void ConfigThread() {
    while (!shutdown_) {
      FileStat newstat(config_file_);
      if (!newstat.exists()) {
        sleep(1);
        continue;
      }
      if (config_stat_.ino() != newstat.ino() ||
          config_stat_.mtime() != newstat.mtime() ||
          config_stat_.size() != newstat.size()) {
        // Config file has been modified
        VLOG(1) << "Config file modified, reloading";
        ReadConfigFile();
        config_stat_ = newstat;
      }
      sleep(1);
    }
    LOG(INFO) << "Configuration thread shutting down";
  }

  void KillConfigThread() {
    shutdown_ = true;
    if (config_thread_.get())
      config_thread_->join();
  }

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
};

}  // namespace openinstrument

#endif  // OPENINSTRUMENT_SERVER_STORE_CONFIG_H_
