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

#include <string>
#include <vector>
#include "lib/common.h"

class ThreadPool : public noncopyable, public Executor {
 public:
  ThreadPool(const string name, int size) : name_(name), size_(size), timeout_(kDefaultTimeout) {}

  ~ThreadPool() {
    if (is_running()) {
      LOG(WARNING) << "Stop thread pool: " << name_ << " in destructor";
      stop();
      LOG(WARNING) << "Stopped thread pool: " << name_ << " in destructor";
    }
  }

  void start() {
    VLOG(1) << name() << " Start, size:" << size_;
    MutexLock locker(run_mutex_);
    if (!threads_.empty()) {
      VLOG(1) << name() << " Running.";
      return;
    }
    for (int i = 0; i < size_; ++i) {
      shared_ptr<thread> t(new thread(bind(&ThreadPool::loop, this, i,
                                           name_.empty() ?  "NoName" : strdup(name_.c_str()))));
      threads_.push_back(t);
    }
  }

  bool is_running() {
    return threads_.size() > 0;
  }

  const string name() const {
    return name_;
  }

  void set_stop_timeout(int timeout) {
    timeout_ = timeout;
  }

  int size() const {
    return size_;
  }

  void stop() {
    VLOG(1) << name() << " Stop.";
    MutexLock locker(run_mutex_);
    if (threads_.empty()) {
      VLOG(1) << name() << " already stop.";
      return;
    }
    for (int i = 0; i < threads_.size(); ++i) {
      pcqueue_.push(boost::function0<void>());
    }
    for (int i = 0; i < threads_.size(); ++i) {
      bool ret = threads_[i]->timed_join(boost::posix_time::seconds(timeout_));
      VLOG(2) << "Join threads: " << i << " ret: " << ret;
    }
    VLOG(1) << name() << " Stopped";
    threads_.clear();
  }

  void push_task(const boost::function0<void> &t) {
    if (t.empty()) {
      LOG(WARNING) << name() <<  " can't push null task.";
      return;
    }
    pcqueue_.push(t);
  }

  void run(const boost::function0<void> &f) {
    push_task(f);
  }

 private:
  void loop(int i, const char *pool_name) {
    VLOG(2) << pool_name << " worker " << i << " start.";
    while (1) {
      VLOG(2) << pool_name << " worker " << i << " wait task.";
      boost::function0<void> h = pcqueue_.Pop();
      if (h.empty()) {
        VLOG(2) << pool_name << " worker " << i << " get empty task, so break.";
        break;
      }
      VLOG(2) << pool_name << " woker " << i << " running task";
      h();
      VLOG(2) << pool_name << " woker " << i << " finish task";
    }
    VLOG(2) << pool_name << " woker " << i << " stop";
    delete pool_name;
  }

  int timeout_;
  vector<shared_ptr<thread> > threads_;
  int size_;
  Mutex run_mutex_;
  Queue<boost::function0<void> > pcqueue_;
  string name_;

  static const int kDefaultTimeout = 60;
};

}  // namespace

#endif  // _OPENINSTRUMENT_LIB_THREADPOOL_H_
