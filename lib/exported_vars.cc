/*
 *  -
 *
 * (c) 2010 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <string>
#include <vector>
#include "lib/common.h"
#include "lib/exported_vars.h"

namespace openinstrument {

///////////// VariableExporter {{{ ////////////////////////////

VariableExporter global_exporter;

VariableExporter::~VariableExporter() {
  if (all_exported_vars_.size())
    LOG(ERROR) << "VariableExporter being destroyed but still contains " << all_exported_vars_.size() << " vars";
}

template <class T> void VariableExporter::add_var(T *var) {
  all_exported_vars_.push_back(var);
}

template <class T> bool VariableExporter::remove_var(T *var) {
  for (vector<ExportedVariable *>::iterator i = all_exported_vars_.begin(); i != all_exported_vars_.end(); ++i) {
    if (*i == var) {
      all_exported_vars_.erase(i);
      return true;
    }
  }
  return false;
}

void VariableExporter::ExportToString(string *output) {
  for (vector<ExportedVariable *>::iterator i = all_exported_vars_.begin(); i != all_exported_vars_.end(); ++i) {
    (*i)->ExportToString(output);
    output->append("\r\n");
  }
}

VariableExporter *VariableExporter::get_global_exporter() {
  return &global_exporter;
}

template <class T> void VariableExporter::export_var(T *var) {
  global_exporter.add_var(var);
}

// }}}
///////////// ExportedVariable {{{ ////////////////////////////

ExportedVariable::~ExportedVariable() {
  if (!global_exporter.remove_var(this))
    LOG(ERROR) << "~ExportedVariable(" << varname_ << ") where variable is not in the global list.";
}

// }}}
///////////// ExportedInteger {{{ ////////////////////////////

ExportedInteger::ExportedInteger(const string &varname, int64_t initial)
  : ExportedVariable(varname),
    counter_(initial) {
  VariableExporter::export_var(this);
}

void ExportedInteger::ExportToString(string *output) const {
  output->append(varname_);
  output->append("\t");
  output->append(lexical_cast<string>(counter_));
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

