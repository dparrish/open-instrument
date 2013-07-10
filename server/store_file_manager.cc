/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <vector>
#include <string>
#include <boost/algorithm/string/predicate.hpp>
#include "lib/common.h"
#include "lib/file.h"
#include "lib/store_config.h"
#include "lib/string.h"
#include "lib/timer.h"
#include "lib/retention_policy_manager.h"
#include "server/store_file_manager.h"
#include "server/indexed_store_file.h"

DECLARE_string(datastore);

namespace openinstrument {

StoreFileManager::StoreFileManager(RetentionPolicyManager &retention_policy_manager)
  : shutdown_(false),
    background_thread_(bind(&StoreFileManager::BackgroundThread, this)),
    retention_policy_manager_(retention_policy_manager) {
  ReglobFiles();
}

StoreFileManager::~StoreFileManager() {
  VLOG(2) << "Shutting down StoreFileManager background thread";
  shutdown_ = true;
  background_thread_.interrupt();
  background_thread_.join();
  for (auto file : store_files_) {
    delete file.second;
  }
  VLOG(2) << "Done shutting down StoreFileManager background thread";
}

vector<string> StoreFileManager::available_files() const {
  MutexLock l(store_files_mutex_);
  vector<string> filenames;
  for (auto &file : store_files_) {
    filenames.push_back(file.first);
  }
  return filenames;
}

int16_t StoreFileManager::num_open_files() const {
  MutexLock l(store_files_mutex_);
  return store_files_.size();
}

const proto::StoreFileHeader &StoreFileManager::OpenFile(const string &filename) {
  scoped_ptr<IndexedStoreFile> file(new IndexedStoreFile(filename));
  if (!file.get())
    throw "Could not open file";
  MutexLock l(store_files_mutex_);
  store_files_[filename] = file.release();
  return store_files_[filename]->header();
}

const proto::StoreFileHeader &StoreFileManager::GetHeader(const string &filename) {
  MutexLock l(store_files_mutex_);
  auto i = store_files_.find(filename);
  if (i == store_files_.end())
    throw "Could not open file";
  return i->second->header();
}

void StoreFileManager::CloseFile(const string &filename) {
  MutexLock l(store_files_mutex_);
  auto i = store_files_.find(filename);
  if (i == store_files_.end())
    return;
  if (i->second->filename == filename) {
    store_files_.erase(i);
    delete i->second;
    return;
  }
}

void StoreFileManager::RunRetentionPolicy() {
  Timer timer;
  timer.Start();
  VLOG(1) << "Locking retention policy";
  MutexLock lock(store_files_mutex_);
  VLOG(1) << "Running retention policy";
  try {
    uint64_t variables = 0;
    uint64_t results = 0;
    for (auto &i : store_files_) {
      auto file = i.second;
      auto &header = file->header();
      for (const auto &varproto : header.variable()) {
        Variable variable(varproto);
        VLOG(1) << "Checking policy for " << variable.ToString();
        bool has_policy = retention_policy_manager_.HasPolicyForVariable(variable);
        if (!has_policy) {
          VLOG(1) << "No policy for " << variable.ToString();
          continue;
        }
        vector<proto::ValueStream> results_set;
        if (!file->GetVariable(variable, &results_set)) {
          VLOG(1) << "Couldn't get variable " << variable.ToString() << " from " << file->filename;
          continue;
        }
        ++variables;
        for (const auto &result : results_set) {
          VLOG(1) << "Variable " << variable.ToString() << " has " << result.value_size() << " results from "
                  << file->filename;
          results += result.value_size();
          if (!result.value_size())
            continue;
          auto &front = result.value(0);
          auto &back = result.value(result.value_size() - 1);
          VLOG(1) << "  between " << Timestamp(front.timestamp()).GmTime() << " and "
                  << Timestamp(back.has_end_timestamp() ? back.end_timestamp() : back.timestamp()).GmTime();
          uint64_t end_timestamp = (back.has_end_timestamp() ?  back.end_timestamp() : back.timestamp());
          auto &policy = retention_policy_manager_.GetPolicy(variable, Timestamp::Now() - end_timestamp);
          string str = StringPrintf("  policy is to %s",
                                    policy.policy() == proto::RetentionPolicyItem::KEEP ? "keep" : "DROP");
          if (policy.mutation_size()) {
            for (auto &mutation : policy.mutation()) {
              switch (mutation.sample_type()) {
                case proto::StreamMutation::AVERAGE:
                  str += " AVERAGE";
                  break;
                case proto::StreamMutation::MIN:
                  str += " MINIMUM";
                  break;
                case proto::StreamMutation::MAX:
                  str += " MAXIMUM";
                  break;
                case proto::StreamMutation::RATE:
                  str += " RATE";
                  break;
                case proto::StreamMutation::RATE_SIGNED:
                  str += " RATE_SIGNED";
                  break;
                case proto::StreamMutation::DELTA:
                  str += " DELTA";
                  break;
                case proto::StreamMutation::LATEST:
                  str += " LATEST";
                  break;
                default:
                  str += " raw samples";
                  break;
              }
              if (mutation.has_sample_frequency())
                str += " every " + Duration(mutation.sample_frequency()).ToString(false);
            }
          } else {
            str += " raw samples";
          }
          if (policy.has_max_age())
            str += " until " + Timestamp(Timestamp::Now() + policy.max_age()).GmTime();
          else
            str += " forever";

          VLOG(1) << str;
        }
      }
    }
    LOG(INFO) << "Finished retention policy over " << variables << " variables and " << results << " values, took "
              << Duration(timer.ms());
  } catch (std::exception &e) {
    LOG(ERROR) << "Exception after " << Duration(timer.ms()) << " while running retention policy: " << e.what();
  }
}

void StoreFileManager::BackgroundThread() {
  VLOG(2) << "StoreFileManager Background thread running";
  FilesystemWatcher watcher;
  watcher.AddWatch(FLAGS_datastore, FilesystemWatcher::FILE_WRITTEN,
                   bind(&StoreFileManager::WatchChanged, this, FLAGS_datastore, _1));
  for (uint64_t ticks = 0; !shutdown_; ticks++) {
    if (ticks % StoreConfig::get().retention_policy().interval() == 0) {
      // Attempt to update files to meet the retention policy
      RunRetentionPolicy();
    }

    sleep(1);
  }
  VLOG(2) << "StoreFileManager Background thread exiting";
}

void StoreFileManager::WatchChanged(const string &path, const string &filename) {
  if (boost::algorithm::starts_with(filename, "datastore.") && boost::algorithm::ends_with(filename, ".bin")) {
    VLOG(2) << "WatchChanged " << path << "/" << filename;
    ReglobFiles();
  }
}

void StoreFileManager::ReglobFiles() {
  vector<string> filenames = Glob(StringPrintf("%s/datastore.*.bin", FLAGS_datastore.c_str()));
  VLOG(2) << "Store filenames:";
  for (auto filename : filenames) {
    if (store_files_.find(filename) != store_files_.end())
      continue;
    VLOG(2) << "  " << filename;
    try {
      OpenFile(filename);
    } catch (std::exception &e) {
      LOG(ERROR) << "    error: " << e.what();
      CloseFile(filename);
    }
  }
}

}  // namespace openinstrument
