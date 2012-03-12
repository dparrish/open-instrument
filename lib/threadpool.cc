/*
 *  -
 *
 * Copyright 2012 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <boost/function.hpp>
#include "lib/common.h"
#include "lib/threadpool.h"

namespace openinstrument {

void ThreadPool::Start() {
  VLOG(1) << "ThreadPool " << name() << " starting";
}

void ThreadPool::Stop() {
  if (threads_.empty())
    return;
  pcqueue_.Shutdown();
  for (size_t i = 0; i < threads_.size(); ++i) {
    bool ret = threads_[i]->timed_join(boost::posix_time::seconds(timeout_));
    if (!ret)
      threads_[i]->detach();
  }
  threads_.clear();
  VLOG(1) << "ThreadPool " << name() << " stopped";
}

void ThreadPool::Add(const Callback &callback) {
  policy_.RunPolicy(threads_.size(), stats_busy_threads_, bind(&ThreadPool::AddThreadCallback, this),
                    bind(&ThreadPool::DeleteThreadCallback, this));
  pcqueue_.Push(callback);
  ++stats_queue_size_;
}

void ThreadPool::AddThreadCallback() {
  VLOG(2) << "ThreadPool " << name() << " creating a thread";
  shared_ptr<thread> t(new thread(bind(&ThreadPool::Loop, this)));
  threads_.push_back(t);
  ++stats_total_threads_;
  ++stats_threads_created_;
}

void ThreadPool::DeleteThreadCallback() {
  VLOG(2) << "ThreadPool " << name() << " deleting a thread";
  // TODO(dparrish): Implement this
  //++stats_threads_deleted_;
}


void ThreadPool::Loop() {
  // Loop forever, trying to pull work off the queue.
  while (true) {
    policy_.RunPolicy(threads_.size(), stats_busy_threads_, bind(&ThreadPool::AddThreadCallback, this),
                      bind(&ThreadPool::DeleteThreadCallback, this));
    try {
      Callback h;
      pcqueue_.WaitAndPop(h);
      ThreadCpuTimer cpu_timer;
      cpu_timer.Start();
      ScopedExportTimer timer(&stats_work_done_);
      ++stats_busy_threads_;
      --stats_queue_size_;
      h();
      --stats_busy_threads_;
      cpu_timer.Stop();
      stats_cpu_used_ += cpu_timer.us();
    } catch (std::out_of_range) {
      break;
    }
  }
}

}  // namespace openinstrument

