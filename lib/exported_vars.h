/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_EXPORTED_VARS_
#define _OPENINSTRUMENT_LIB_EXPORTED_VARS_

#include <string>
#include <vector>
#include "lib/atomic.h"
#include "lib/common.h"
#include "lib/timer.h"

namespace openinstrument {


// Abstract class which is the parent of all exported values.
// This superclass takes care of adding the new object to the global variable exporter, but not much else.
// NOTE: Do not use this in your code.
class ExportedVariable : private noncopyable {
 public:
  explicit ExportedVariable(const string &varname) : varname_(varname) {}
  virtual ~ExportedVariable();
  virtual void ExportToString(string *output) const = 0;

 protected:
  string varname_;
};

// Single value atomic integer exported value.
// This is the base of most of the other exported values, but is quite useful on its own. It exports a single int64
// value which can be incremented using the standard math operators.
// Example use:
//   ExportedInteger count("/test/counter");
//   ++count;
//   count += 5;
//   LOG(INFO) << "Current value is " << count;
class ExportedInteger : public ExportedVariable {
 public:
  explicit ExportedInteger(const string &varname, int64_t initial = 0);
  virtual ~ExportedInteger() {}
  void operator=(int64_t value);
  int64_t operator++();
  int64_t operator+=(int64_t incby);
  int64_t operator--();
  operator int64_t() const;
  virtual void ExportToString(string *output) const;

 private:
  ExportedInteger(const ExportedInteger &);
  ExportedInteger &operator=(const ExportedInteger &);

  AtomicInt64 counter_;
};


// An exported ratio class.
// This results in three variables being exported:
//   <your var name>-total      -- the sum of success and failure counts
//   <your var name>-success    -- the count of successes
//   <your var name>-failure    -- the count of failures
class ExportedRatio {
 public:
  explicit ExportedRatio(const string &varname);
  void success();
  void failure();

 private:
  ExportedInteger total_;
  ExportedInteger success_;
  ExportedInteger failure_;
};


// An exported average class.
// This results in two variables being exported:
//   <your var name>-total-count    -- the total number of times that update() is called
//   <your var name>-overall-sum    -- the sum of all time taken up at each update() call.
// The combination of these two variables allows you to get an average with:
//   var-overall-sum / var-total-count.
class ExportedAverage {
 public:
  explicit ExportedAverage(const string &varname);
  void update(int64_t sum, int64_t count = 1);
  int64_t overall_sum() const;
  int64_t total_count() const;

 private:
  ExportedInteger total_count_;
  ExportedInteger overall_sum_;
};


// An exported timer class.
// The timer should be updated using the Timer class which contains milliseconds, and a total count will also be
// incremented.
class ExportedTimer : public ExportedAverage {
 public:
  explicit ExportedTimer(const string &varname) : ExportedAverage(varname) {}
  inline void update(const Timer &timer) {
    ExportedAverage::update(timer.ms(), static_cast<int64_t>(1));
  }
};


// Convenience function to automatically record the duration of a section of code.
// This will start a timer when the object is created. When the object is destroyed, the ExportedTimer will be updated
// with the time between creation and destruction.
// Example use:
//   ExportedTimer timer("/test/timer");
//   if (true) {
//     ScopedExportTimer t(&timer);
//     sleep(15);
//   }
// At this point, timer will be updated with a count of 1 and a time of 15seconds.
// To complete the timing earlier than destruction, call the stop() method.
// To discard the timing completely, call the cancel() method.
class ScopedExportTimer : private noncopyable {
 public:
  explicit ScopedExportTimer(ExportedTimer *exptimer);
  ~ScopedExportTimer();

  // Stop the timer and record the time and run record.
  void stop();

  // Stop the timer and do NOT record the time.
  void cancel();

 private:
  Timer timer_;
  ExportedTimer *exptimer_;
  bool completed_;
};


class ExportedIntegerSet {
 public:
  explicit ExportedIntegerSet() {}
  // Create an ExportedIntegerSet with a prefix that will be prepended to all variables set
  explicit ExportedIntegerSet(const string &prefix);
  ~ExportedIntegerSet();

  // Retrieve an ExportedInteger reference by name
  ExportedInteger &operator[](const string &varname);

 private:
  typedef unordered_map<string, ExportedInteger *> counter_map;
  string prefix_;
  counter_map variables_;
};


// This singleton class takes care of containing and exporting all ExportedVariable objects.
// NOTE: This should generally not be used in your own code, except perhaps for
//       get_global_exporter()->ExportToString() to render the values of each variable.
class VariableExporter {
 public:
  ~VariableExporter();
  // Add a variable reference to the exporter. This is called by ExporterVariable's constructor.
  template <class T> void add_var(T *var);
  // Remove a variable reference from the exporter. This is called by ExporterVariable's destructor.
  template <class T> bool remove_var(T *var);

  // Convenience method that adds a variable to the global exporter.
  template <class T> static void export_var(T *var);

  // Render every contained variable to the passed in string. The output is in the internal OpenInstrument variable
  // format.
  void ExportToString(string *output);

  // Return the global VariableExporter singleton instance
  static VariableExporter *get_global_exporter();

 private:
  vector<ExportedVariable *> all_exported_vars_;
};


}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_EXPORTED_VARS_
