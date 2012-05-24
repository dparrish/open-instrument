/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <fstream>
#include <ios>
#include <iostream>
#include <string>
#include <vector>
#include <boost/thread/once.hpp>
#include <boost/thread.hpp>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <unistd.h>
#include "lib/common.h"
#include "lib/exported_vars.h"
#include "lib/protobuf.h"
#include "lib/store_client.h"
#include "lib/variable.h"

namespace openinstrument {

DEFINE_string(datastore_address, "", "Host and port of datastore, where all exported vars will be sent");

///////////// VariableExporter {{{ ////////////////////////////

VariableExporter global_exporter;
boost::once_flag global_exporter_flag = BOOST_ONCE_INIT;

VariableExporter::~VariableExporter() {
  if (all_exported_vars_.size())
    LOG(ERROR) << "VariableExporter being destroyed but still contains " << all_exported_vars_.size() << " vars";
  if (export_thread_ != NULL) {
    LOG(INFO) << "Waiting for variable export thread to shut down";
    export_thread_->interrupt();
    export_thread_->join();
  }
  ExclusiveLock lock(mutex_);
}

void VariableExporter::AddVar(ExportedVariable *var) {
  ExclusiveLock lock(mutex_);
  all_exported_vars_.push_back(var);
}

bool VariableExporter::RemoveVar(ExportedVariable *var) {
  ExclusiveLock lock(mutex_);
  for (vector<ExportedVariable *>::iterator i = all_exported_vars_.begin(); i != all_exported_vars_.end(); ++i) {
    if (*i == var) {
      all_exported_vars_.erase(i);
      return true;
    }
  }
  return false;
}

void VariableExporter::ExportToString(string *output) {
  SharedLock lock(mutex_);
  RunCallbacks();
  for (ExportedVariable *i : all_exported_vars_) {
    proto::ValueStream stream;
    i->ExportToValueStream(&stream);
    Variable var(stream.variable());
    for (unordered_map<string, string>::iterator j = extra_labels_.begin(); j != extra_labels_.end(); ++j) {
      var.SetLabel(j->first, j->second);
    }
    output->append(var.ToString());
    output->append("\t");
    for (auto &value : stream.value()) {
      if (i > 0)
        output->append("\t");
      if (value.has_double_value())
        output->append(lexical_cast<string>(value.double_value()));
      else if (value.has_string_value())
        output->append(value.string_value());
    }
    output->append("\n");
  }
}

void VariableExporter::ExportToStore(StoreClient *client) {
  proto::AddRequest req;
  RunCallbacks();
  for (ExportedVariable *i : all_exported_vars_) {
    proto::ValueStream *stream = req.add_stream();
    i->ExportToValueStream(stream);
    Variable var(stream->variable());
    for (unordered_map<string, string>::iterator j = extra_labels_.begin(); j != extra_labels_.end(); ++j) {
      var.SetLabel(j->first, j->second);
    }
    var.ToProtobuf(stream->mutable_variable());
  }
  try {
    scoped_ptr<proto::AddResponse> response(client->Add(req));
  } catch (exception &e) {
    LOG(WARNING) << "Unable to export vars to the datastore: " << e.what();
  }
}

void VariableExporter::ExportToStore(string server) {
  if (server.empty())
    server = FLAGS_datastore_address;
  if (server.empty()) {
    LOG(WARNING) << "No datastore address defined, all recorded statistics will be lost.";
    return;
  }
  StoreClient client(server);
  return ExportToStore(&client);
}

VariableExporter *VariableExporter::GetGlobalExporter() {
  return &global_exporter;
}

void VariableExporter::ExportVar(ExportedVariable *var) {
  global_exporter.AddVar(var);
}

void VariableExporter::StartExportThread(const string &server, uint64_t interval) {
  if (export_thread_ != NULL)
    return;
  export_thread_ = new thread(bind(&VariableExporter::ExportThread, this,
                                   server.size() ? server : FLAGS_datastore_address, interval));
}

void VariableExporter::ExportThread(const string &server, uint64_t interval) {
  StoreClient client(server);
  while (true) {
    try {
      sleep(interval);
    } catch (boost::thread_interrupted) {
      // Clean exit
      return;
    }
    boost::this_thread::disable_interruption di;
    ExportToStore(&client);
  }
}

void VariableExporter::SetExportLabel(const string &label, const string &value) {
  extra_labels_[label] = value;
}

void VariableExporter::ClearExportLabel(const string &label) {
  extra_labels_.erase(label);
}

void VariableExporter::AddExportCallback(const Callback &callback) {
  pre_export_callbacks_.push_back(callback);
}

void VariableExporter::RunCallbacks() {
  for (vector<Callback>::const_iterator i = pre_export_callbacks_.begin(); i != pre_export_callbacks_.end();
       ++i) {
    (*i)();
  }
}

// }}}
///////////// ExportedVariable {{{ ////////////////////////////

ExportedVariable::~ExportedVariable() {
  if (!global_exporter.RemoveVar(this))
    LOG(ERROR) << "~ExportedVariable(" << variable().ToString() << ") where variable is not in the global list.";
}

// }}}
///////////// ExportedInteger {{{ ////////////////////////////

ExportedInteger::ExportedInteger(const Variable &varname, int64_t initial)
  : ExportedVariable(varname),
    counter_(initial) {
  VariableExporter::ExportVar(this);
}

void ExportedInteger::ExportToValueStream(proto::ValueStream *stream) const {
  variable().ToProtobuf(stream->mutable_variable());
  proto::Value *value = stream->add_value();
  value->set_timestamp(Timestamp::Now());
  value->set_double_value(lexical_cast<double>(counter_));
}

void ExportedInteger::operator=(int64_t value) {
  counter_ = value;
}

int64_t ExportedInteger::operator++() {
  return ++counter_;
}

int64_t ExportedInteger::operator+=(int64_t incby) {
  return counter_ += incby;
}

int64_t ExportedInteger::operator--() {
  return --counter_;
}

ExportedInteger::operator int64_t() const {
  return counter_;
}

// }}}
///////////// ScopedExportTimer {{{ ////////////////////////////

ScopedExportTimer::ScopedExportTimer(ExportedTimer *exptimer)
  : exptimer_(exptimer),
    completed_(false) {
  timer_.Start();
}
ScopedExportTimer::~ScopedExportTimer() {
  stop();
}

void ScopedExportTimer::stop() {
  if (!completed_) {
    completed_ = true;
    timer_.Stop();
    exptimer_->update(timer_);
  }
}

void ScopedExportTimer::cancel() {
  completed_ = true;
}

// }}}
///////////// ExportedIntegerSet {{{ ////////////////////////////

ExportedIntegerSet::ExportedIntegerSet(const string &prefix) : prefix_(prefix) {
  // Trim any trailing / from the prefix
  if (prefix_[prefix_.size() - 1] == '/')
    prefix_.erase(prefix_.size() - 2);
}

ExportedIntegerSet::~ExportedIntegerSet() {
  for (counter_map::iterator i = variables_.begin(); i != variables_.end(); ++i) {
    delete i->second;
  }
}

ExportedInteger &ExportedIntegerSet::operator[](const string &varname) {
  string fullvar;
  if (!prefix_.size()) {
    if (varname[0] == '/')
      fullvar = varname;
    else
      fullvar = "/" + varname;
  } else {
    if (varname[0] == '/')
      fullvar = prefix_ + varname;
    else
      fullvar = prefix_ + "/" + varname;
  }
  counter_map::iterator i(variables_.find(fullvar));
  if (i == variables_.end()) {
    ExportedInteger *counter = new ExportedInteger(fullvar);
    variables_[fullvar.c_str()] = counter;
    return *variables_[fullvar.c_str()];
  }
  return *i->second;
}

// }}}
///////////// ExportedRatio {{{ ////////////////////////////

ExportedRatio::ExportedRatio(const Variable &varname)
  : total_(varname.CopyAndAppend("-total")),
    success_(varname.CopyAndAppend("-success")),
    failure_(varname.CopyAndAppend("-failure")) {}

void ExportedRatio::success() {
  ++total_;
  ++success_;
}

void ExportedRatio::failure() {
  ++total_;
  ++failure_;
}

// }}}
///////////// ExportedAverage {{{ ////////////////////////////

ExportedAverage::ExportedAverage(const Variable &varname)
  : total_count_(varname.CopyAndAppend("-total-count")),
    overall_sum_(varname.CopyAndAppend("-overall-sum")) {
  total_count_.mutable_variable()->set_rate();
  overall_sum_.mutable_variable()->set_rate();
}

void ExportedAverage::update(int64_t sum, int64_t count) {
  total_count_ += count;
  overall_sum_ += sum;
}

int64_t ExportedAverage::overall_sum() const {
  return overall_sum_;
}

int64_t ExportedAverage::total_count() const {
  return total_count_;
}

// }}}
///////////// ExportedString {{{ ////////////////////////////

ExportedString::ExportedString(const Variable &varname) : ExportedVariable(varname) {
  VariableExporter::ExportVar(this);
}

void ExportedString::ExportToValueStream(proto::ValueStream *stream) const {
  variable().ToProtobuf(stream->mutable_variable());
  proto::Value *value = stream->add_value();
  value->set_timestamp(Timestamp::Now());
  value->set_string_value(value_);
}

void ExportedString::operator=(string value) {
  value_ = value;
}

// }}}
///////////// Global Exported Stats {{{ ////////////////////////////

// These stats should exist for every process that runs with the OpenInstrument monitoring enabled.
class GlobalExportedStats {
 public:
  GlobalExportedStats()
    : stats_cpu_usage("/openinstrument/process/cpu-usage{units=ms}"),
      stats_run_time("/openinstrument/process/uptime{units=ms}"),
      stats_vm_usage("/openinstrument/process/total-memory"),
      stats_rss("/openinstrument/process/rss"),
      stats_nicelevel("/openinstrument/process/nice-level"),
      stats_pid("/openinstrument/process/pid"),
      stats_utime("/openinstrument/process/utime{units=ms}"),
      stats_stime("/openinstrument/process/stime{units=ms}"),
      stats_num_threads("/openinstrument/process/num-threads"),
      stats_open_fds("/openinstrument/process/filedescriptor-count"),
      stats_os_name("/openinstrument/process/os-name"),
      stats_os_version("/openinstrument/process/os-version"),
      stats_os_arch("/openinstrument/process/os-arch"),
      stats_nodename("/openinstrument/process/nodename"),
      stats_cpuset("/openinstrument/process/cpuset"),
      start_time(Timestamp::Now()) {
    VariableExporter::GetGlobalExporter()->AddExportCallback(bind(&GlobalExportedStats::Update, this));
    cpu_usage.Start();
  }
 private:
  void Update() {
    stats_cpu_usage = cpu_usage.ms();
    stats_run_time = Timestamp::Now() - start_time;
    // Read /proc/self/stat to get process stats
    std::ifstream stat_stream("/proc/self/stat", std::ios_base::in);
    string comm, state, ppid, pgrp, session, tty_nr, tpgid, flags, minflt, cminflt, majflt, cmajflt;
    string cutime, cstime, itrealvalue, starttime;
    uint64_t pid, vsize, rss, utime, stime, priority, nice, num_threads;
    stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr >> tpgid >> flags >> minflt >> cminflt
                >> majflt >> cmajflt >> utime >> stime >> cutime >> cstime >> priority >> nice >> num_threads
                >> itrealvalue >> starttime >> vsize >> rss;
    stat_stream.close();
    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
    stats_pid = pid;
    stats_utime = static_cast<uint64_t>(static_cast<double>(utime / sysconf(_SC_CLK_TCK)) * 1000);
    stats_stime = static_cast<uint64_t>(static_cast<double>(stime / sysconf(_SC_CLK_TCK)) * 1000);
    stats_nicelevel = nice;
    stats_num_threads = num_threads;
    stats_vm_usage = vsize / 1024;
    stats_rss = rss * page_size_kb;

    // Get uname values
    struct utsname buf;
    uname(&buf);
    stats_os_name = buf.sysname;
    stats_os_version = buf.release;
    stats_nodename = buf.nodename;
    stats_os_arch = buf.machine;

    {
      // Record current cpuset.
      // If cpusets are not being used, this will just give '/'
      File file("/proc/self/cpuset");
      char buf[64] = {0};
      file.Read(buf, 63);
      for (char *p = buf; p && *p; p++) {
        if (*p == '\n') {
          *p = 0;
          break;
        }
      }
      stats_cpuset = buf;
    }

    {
      // Count open file descriptors
      stats_open_fds = 0;
      struct stat sb;
      for (int i = 0; i <= getdtablesize(); i++) {
        fstat(i, &sb);
        if (errno != EBADF)
          ++stats_open_fds;
      }
    }
  }

  ExportedInteger stats_cpu_usage;
  ExportedInteger stats_run_time;
  ExportedInteger stats_vm_usage;
  ExportedInteger stats_rss;
  ExportedInteger stats_nicelevel;
  ExportedInteger stats_pid;
  ExportedInteger stats_utime;
  ExportedInteger stats_stime;
  ExportedInteger stats_num_threads;
  ExportedInteger stats_open_fds;
  ExportedString stats_os_name;
  ExportedString stats_os_version;
  ExportedString stats_os_arch;
  ExportedString stats_nodename;
  ExportedString stats_cpuset;
  ProcessCpuTimer cpu_usage;
  uint64_t start_time;
} global_exported_stats;

// }}}

}  // namespace openinstrument

