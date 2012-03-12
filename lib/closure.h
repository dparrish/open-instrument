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

namespace openinstrument {

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
  ~BackgroundExecutor() {
    JoinThreads();
  }

  virtual void Add(const Callback &callback) {
    threads_.push_back(new boost::thread(callback));
    TryJoinThreads();
  }

  void TryJoinThreads() {
    for (std::vector<boost::thread *>::iterator i = threads_.begin(); i != threads_.end(); ++i) {
      if ((*i)->timed_join(boost::posix_time::seconds(0))) {
        delete *i;
        threads_.erase(i);
        return TryJoinThreads();
      }
    }
  }

  void JoinThreads() {
    for (std::vector<boost::thread *>::iterator i = threads_.begin(); i != threads_.end(); ++i) {
      (*i)->join();
      delete *i;
    }
    threads_.clear();
  }

 private:
  std::vector<boost::thread *> threads_;
};

}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_COMMON_H_
