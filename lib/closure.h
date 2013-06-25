/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_CLOSURE_H_
#define _OPENINSTRUMENT_LIB_CLOSURE_H_

#include <boost/function.hpp>
#include <boost/timer.hpp>
#include "lib/common.h"
#include "lib/timer.h"

namespace openinstrument {

class Timestamp;

typedef boost::function<void()> Callback;

// Base Executor class, just runs the callback passed to Add() immediately.
// Not very useful on its own, should be subclassed.
class Executor {
 public:
  Executor() {}
  virtual void Add(const Callback &callback) {
    LOG(INFO) << "Default Executor running callback";
    callback();
  }
};

// This Executor will start individual threads as required, and wait for all background threads to exit when this object
// is destroyed.
class BackgroundExecutor : public Executor {
 public:
  ~BackgroundExecutor();
  virtual void Add(const Callback &callback);
  void TryJoinThreads();
  void JoinThreads();

 private:
  std::vector<boost::thread *> threads_;
};

// Contains a set of callbacks that are each run after a specified time.
class DelayedExecutor : public Executor {
 public:
  // Add a callback to be run at a later time.
  // If an Executor is provided, the callback will be passed to it when the time has come that it should be run. The
  // Executor *may* then delay running the code.
  //
  // If the time given is in the past, the callback is run immediately.
  void Add(const Callback &callback, Timestamp when, Executor *executor = NULL);
  virtual void Add(const Callback &callback);

  // Wait for all callbacks to be scheduled.
  // Note that this does not wait for all callbacks to be run. This simply sleeps until the time that the last existing
  // callback should have started.
  void WaitForAll() const;

  // Shut down the background processing thread.
  // This will not run any pending callbacks.
  // This is run by the destructor.
  void Stop();

  // Start the background thread.
  // This is run by the constructor.
  void Start();

  DelayedExecutor();
  virtual ~DelayedExecutor();

 private:
  void WakeBackgroundThread();
  void BackgroundThread();

  class TimedCallback {
   public:
    Callback callback;
    uint64_t run_time;
    Executor *executor;
    int operator<(const TimedCallback &other) const {
      return run_time < other.run_time;
    }
  };

  bool stop_;
  boost::thread *thread_;
  std::multiset<TimedCallback> callbacks_;
};


}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_COMMON_H_
