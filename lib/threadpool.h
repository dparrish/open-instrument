/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_THREADPOOL_H_
#define _OPENINSTRUMENT_LIB_THREADPOOL_H_

#include <string>
#include <vector>
#include "lib/atomic.h"
#include "lib/common.h"
#include "lib/closure.h"
#include "lib/exported_vars.h"
#include "lib/timer.h"

namespace openinstrument {

class ThreadPoolPolicy {
 public:
  virtual ~ThreadPoolPolicy() {}
  virtual void RunPolicy(int total_threads, int busy_threads, const Callback &add_thread_callback,
                         const Callback &delete_thread_callback) = 0;
};

// Provides a "safe" thread pool policy with a target size and a maximum size.
// If there are not enough threads available to service a request, one will be created up until the max size is reached.
// Idle threads are slowly deleted (one every 20 seconds) to keep the number of threads around the target.
class DefaultThreadPoolPolicy : public ThreadPoolPolicy {
 public:
  explicit DefaultThreadPoolPolicy(int target_size, int max_size)
    : target_size_(target_size),
      max_size_(max_size) {
    CHECK(target_size_);
    if (max_size_ == 0)
      max_size_ = target_size;
    CHECK(target_size_ <= max_size_);
    delete_timer_.Start();
  }
  virtual ~DefaultThreadPoolPolicy() {}
  virtual void RunPolicy(int total_threads, int busy_threads, const Callback &add_thread_callback,
                         const Callback &delete_thread_callback) {
    if (total_threads > target_size_ && busy_threads < total_threads) {
      delete_timer_.Stop();
      if (delete_timer_.seconds() > 20) {
        // Can remove at most one idle thread every 20 seconds
        delete_thread_callback();
        delete_timer_.Start();
      }
    }
    if (total_threads >= max_size_)
      return;
    if (busy_threads >= total_threads) {
      add_thread_callback();
      delete_timer_.Start();
      total_threads++;
    }
    if (total_threads < target_size_) {
      delete_timer_.Start();
      add_thread_callback();
      total_threads++;
    }
  }

 private:
  int target_size_;
  int max_size_;
  Timer delete_timer_;
};

class ThreadPool : public Executor {
 public:
  ThreadPool(const string name, ThreadPoolPolicy &policy)
    : name_(name),
      timeout_(60),
      policy_(policy),
      stats_total_threads_(
          StringPrintf("/openinstrument/thread-pool/total-threads{thread-pool=%s}", name_.c_str())),
      stats_busy_threads_(
          StringPrintf("/openinstrument/thread-pool/busy-threads{thread-pool=%s}", name_.c_str())),
      stats_threads_created_(
          StringPrintf("/openinstrument/thread-pool/threads-created{thread-pool=%s}", name_.c_str())),
      stats_threads_deleted_(
          StringPrintf("/openinstrument/thread-pool/threads-deleted{thread-pool=%s}", name_.c_str())),
      stats_queue_size_(
          StringPrintf("/openinstrument/thread-pool/queue-size{thread-pool=%s}", name_.c_str())),
      stats_work_done_(
          StringPrintf("/openinstrument/thread-pool/work-done{thread-pool=%s}", name_.c_str())),
      stats_cpu_used_(
          StringPrintf("/openinstrument/thread-pool/cpu-used{thread-pool=%s,units=us}", name_.c_str())) {

    Start();
  }

  ~ThreadPool() {
    Stop();
  }

  // Add work to the thread pool. This will wake up a single thread to perform the work if there are any threads
  // available.
  virtual void Add(const Callback &callback);
  void Stop();

  const string name() const {
    return name_;
  }

  void set_stop_timeout(int timeout) {
    timeout_ = timeout;
  }

 private:
  void Start();
  void Loop();
  void AddThreadCallback();
  void DeleteThreadCallback();

  string name_;
  int timeout_;
  vector<shared_ptr<thread> > threads_;
  ThreadPoolPolicy &policy_;
  Queue<Callback> pcqueue_;
  ExportedInteger stats_total_threads_;
  ExportedInteger stats_busy_threads_;
  ExportedInteger stats_threads_created_;
  ExportedInteger stats_threads_deleted_;
  ExportedInteger stats_queue_size_;
  ExportedTimer stats_work_done_;
  ExportedInteger stats_cpu_used_;
};

}  // namespace

#endif  // _OPENINSTRUMENT_LIB_THREADPOOL_H_
