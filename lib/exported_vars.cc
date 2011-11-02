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
#include "lib/common.h"
#include "lib/exported_vars.h"
#include "lib/protobuf.h"
#include "lib/store_client.h"

namespace openinstrument {

DEFINE_string(datastore_address, "", "Host and port of datastore, where all exported vars will be sent");

///////////// VariableExporter {{{ ////////////////////////////

VariableExporter global_exporter;

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
  for (vector<ExportedVariable *>::iterator i = all_exported_vars_.begin(); i != all_exported_vars_.end(); ++i) {
    (*i)->ExportToString(output);
    output->append("\r\n");
  }
}

void VariableExporter::ExportToStore(StoreClient *client) {
  proto::AddRequest req;
  BOOST_FOREACH(ExportedVariable *i, all_exported_vars_) {
    proto::ValueStream *stream = req.add_stream();
    i->ExportToValueStream(stream);
    Variable var(stream->variable());
    for (unordered_map<string, string>::iterator j = extra_labels_.begin(); j != extra_labels_.end(); ++j) {
      var.SetLabel(j->first, j->second);
    }
    var.CopyTo(stream->mutable_variable());
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

// }}}
///////////// ExportedVariable {{{ ////////////////////////////

ExportedVariable::~ExportedVariable() {
  if (!global_exporter.RemoveVar(this))
    LOG(ERROR) << "~ExportedVariable(" << variable().ToString() << ") where variable is not in the global list.";
}

// }}}
///////////// ExportedInteger {{{ ////////////////////////////

ExportedInteger::ExportedInteger(const string &varname, int64_t initial)
  : ExportedVariable(varname),
    counter_(initial) {
  VariableExporter::ExportVar(this);
}

void ExportedInteger::ExportToString(string *output) const {
  output->append(variable().ToString());
  output->append("\t");
  output->append(lexical_cast<string>(counter_));
}

void ExportedInteger::ExportToValueStream(proto::ValueStream *stream) const {
  variable().CopyTo(stream->mutable_variable());
  proto::Value *value = stream->add_value();
  value->set_timestamp(Timestamp::Now());
  value->set_value(lexical_cast<double>(counter_));
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

ExportedRatio::ExportedRatio(const string &varname)
  : total_(varname + "-total"),
    success_(varname + "-success"),
    failure_(varname + "-failure") {}

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

ExportedAverage::ExportedAverage(const string &varname)
  : total_count_(varname + "-total-count"),
    overall_sum_(varname + "-overall-sum") {}

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

}  // namespace openinstrument

