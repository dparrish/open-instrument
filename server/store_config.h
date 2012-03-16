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
#include "lib/file.h"
#include "lib/openinstrument.pb.h"
#include "lib/protobuf.h"
#include "lib/store_client.h"
#include "lib/exported_vars.h"

namespace openinstrument {

class StoreConfig : private noncopyable {
 public:
  StoreConfig() : config_thread_(NULL), shutdown_(false) {}

  explicit StoreConfig(const string &config_file)
    : config_file_(config_file),
      config_thread_(NULL),
      shutdown_(false) {
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

  void ConfigThread() {
    FileStat stat("/dev/null");
    while (!shutdown_) {
      FileStat newstat(config_file_);
      if (!newstat.exists()) {
        sleep(1);
        continue;
      }
      if (stat.ino() != newstat.ino() || stat.mtime() != newstat.mtime() || stat.size() != newstat.size()) {
        // Config file has been modified
        VLOG(1) << "Config file modified, reloading";
        ReadConfigFile();
        DistributeConfig();
        stat = newstat;
        continue;
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

  bool HandleNewConfig(const proto::StoreConfig &config) {
    MutexLock lock(mutex_);
    if (config.last_update() > config_.last_update()) {
      // Config is newer than current, save it
      config_.CopyFrom(config);
      WriteConfigFile();
      VLOG(1) << "Loaded new configuration";
      return true;
    } else if (config.last_update() == config_.last_update()) {
      return false;
    } else {
      LOG(WARNING) << "Discarding new config with older timestamp";
    }
    return false;
  }

  bool ReadConfig(const string &input) {
    proto::StoreConfig new_config;
    if (google::protobuf::TextFormat::MergeFromString(input, &new_config)) {
      return HandleNewConfig(new_config);
    }
    LOG(WARNING) << "Error reading new config";
    return false;
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
    MutexLock lock(mutex_);
    bool found = false;
    LOG(INFO) << "Setting state for " << address << " to " << state;
    for (int i = 0 ; i < config_.server_size(); i++) {
      proto::StoreServer *server = config_.mutable_server(i);
      if (server->address() == address) {
        server->set_state(state);
        found = true;
      }
    }
    if (found) {
      SetLastUpdateTime();
      DistributeConfig();
    }
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
    initial_load_notify_.WaitForNotification();
    sleep(10);
  }

  void WriteConfigFile() const {
    if (config_file_.empty())
      return;
    string output;
    DumpConfig(&output);
    File fh(config_file_, "w");
    fh.Write(output);
    LOG(INFO) << "Wrote config file to " << config_file_;
  }

  void DumpConfig(string *output) const {
    google::protobuf::TextFormat::PrintToString(config_, output);
  }

  void DistributeConfig() {
    for (int i = 0 ; i < config_.server_size(); i++) {
      const proto::StoreServer &server = config_.server(i);
      PushConfig(server.address());
    }
  }

  void PushConfig(const string &address) {
    try {
      VLOG(3) << "Pushing config to server " << address;
      StoreClient client(address);
      scoped_ptr<proto::StoreConfig> response(client.PushStoreConfig(config_));
      StoreConfig other_config;
      other_config.HandleNewConfig(*response);
      if (HandleNewConfig(*response)) {
        LOG(INFO) << "Pushed config to " << address << " which had a newer config";
      }
    } catch (const exception &e) {
      LOG(WARNING) << "Couldn't push config to " << address << ", trying again later: " << e.what();
    }
  }

  const proto::StoreConfig &config() const {
    return config_;
  }

  void SetLastUpdateTime() {
    config_.set_last_update(Timestamp::Now());
  }

 private:
  proto::StoreConfig config_;
  string config_file_;
  mutable Mutex mutex_;
  scoped_ptr<thread> config_thread_;
  bool shutdown_;
  vector<Callback> reload_callbacks_;
  Notification initial_load_notify_;
};

}  // namespace openinstrument

#endif  // OPENINSTRUMENT_SERVER_STORE_CONFIG_H_
