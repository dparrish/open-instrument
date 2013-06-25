/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <string>
#include <vector>
#include <google/protobuf/text_format.h>
#include "lib/common.h"
#include "lib/exported_vars.h"
#include "lib/file.h"
#include "lib/hash.h"
#include "lib/openinstrument.pb.h"
#include "lib/protobuf.h"
#include "lib/store_client.h"
#include "lib/store_config.h"

DEFINE_string(config_file, "config.txt", "Configuration file to read on startup");

namespace openinstrument {

StoreConfig *StoreConfig::global_config_ = NULL;
Mutex StoreConfig::global_config_mutex_;

proto::StoreConfig &StoreConfig::get() {
  return get_manager().config();
}

StoreConfig &StoreConfig::get_manager() {
  MutexLock l(global_config_mutex_);
  if (!global_config_) {
    global_config_ = new StoreConfig(FLAGS_config_file.c_str());
    global_config_->WaitForConfigLoad();
  }
  return *global_config_;
}

StoreConfig::StoreConfig(const string &filename)
  : config_file_(filename),
    config_thread_(NULL),
    shutdown_(false),
    config_stat_("/dev/null"),
    ring_(2) {
  ReadConfigFile();
  WaitForConfigLoad();
  config_thread_.reset(new thread(bind(&StoreConfig::ConfigThread, this)));
}

StoreConfig::~StoreConfig() {
  KillConfigThread();
}

void StoreConfig::SetConfigFilename(const string &filename) {
  MutexLock l(global_config_mutex_);
  config_file_ = filename;
  ReadConfigFile();
  WaitForConfigLoad();
}

void StoreConfig::AddReloadCallback(const Callback &callback) {
  reload_callbacks_.push_back(callback);
}

void StoreConfig::RunCallbacks() {
  for (vector<Callback>::const_iterator i = reload_callbacks_.begin(); i != reload_callbacks_.end(); ++i) {
    (*i)();
  }
}

void StoreConfig::set_config(const proto::StoreConfig &config) {
  config_.CopyFrom(config);
}

void StoreConfig::HandleNewConfig(const proto::StoreConfig &config) {
  VLOG(1) << "Loaded new configuration with changes";
  MutexLock lock(mutex_);
  config_.CopyFrom(config);
  UpdateHashRing();
  RunCallbacks();
}

bool StoreConfig::ReadConfig(const string &input) {
  proto::StoreConfig new_config;
  if (google::protobuf::TextFormat::MergeFromString(input, &new_config)) {
    HandleNewConfig(new_config);
    return true;
  }
  LOG(WARNING) << "Error reading new config";
  return false;
}

void StoreConfig::UpdateHashRing() {
  ring_.Clear();
  for (auto &server : config_.server()) {
    ring_.AddNode(server.address());
  }
}

const StoreConfig::HashRingType &StoreConfig::hash_ring() const {
  return ring_;
}

proto::StoreServer::State StoreConfig::GetServerState(const string &address) const {
  MutexLock lock(mutex_);
  for (auto &server : config_.server()) {
    if (server.address() == address) {
      return server.state();
    }
  }
  return proto::StoreServer::UNKNOWN;
}

void StoreConfig::SetServerState(const string &address, proto::StoreServer::State state) {
  VLOG(3) << "Setting state for " << address << " to " << state;
  mutex_.lock();
  for (int i = 0 ; i < config_.server_size(); i++) {
    proto::StoreServer *server = config_.mutable_server(i);
    if (server->address() == address) {
      server->set_state(state);
      server->set_last_updated(Timestamp::Now());
      mutex_.unlock();
      return;
    }
  }
  mutex_.unlock();
  LOG(INFO) << "Done setting server state";
}

proto::StoreServer *StoreConfig::server(const string &address) {
  for (int i = 0 ; i < config_.server_size(); i++) {
    proto::StoreServer *server = config_.mutable_server(i);
    if (server->address() == address)
      return server;
  }
  return NULL;
}

bool StoreConfig::ReadConfigFile() {
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

void StoreConfig::WaitForConfigLoad() {
  if (config_file_.empty())
    return;
  initial_load_notify_.WaitForNotification();
}

void StoreConfig::WriteConfigFile() {
  if (config_file_.empty())
    return;
  File fh(config_file_, "w");
  string output = DumpConfig();
  fh.Write(output);
  fh.Close();
  config_stat_ = FileStat(config_file_);
}

string StoreConfig::DumpConfig() const {
  string output;
  MutexLock lock(mutex_);
  google::protobuf::TextFormat::PrintToString(config_, &output);
  return output;
}

proto::StoreConfig &StoreConfig::config() {
  return config_;
}

void StoreConfig::ConfigThread() {
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

void StoreConfig::KillConfigThread() {
  shutdown_ = true;
  if (config_thread_.get())
    config_thread_->join();
}

}  // namespace openinstrument
