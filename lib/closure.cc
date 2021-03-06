/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <boost/function.hpp>
#include "lib/closure.h"

namespace openinstrument {

BackgroundExecutor::~BackgroundExecutor() {
  JoinThreads();
}

void BackgroundExecutor::Add(const Callback &callback) {
  threads_.push_back(new boost::thread(callback));
  TryJoinThreads();
}

void BackgroundExecutor::TryJoinThreads() {
  for (std::vector<boost::thread *>::iterator i = threads_.begin(); i != threads_.end(); ++i) {
    if ((*i)->timed_join(boost::posix_time::seconds(0))) {
      delete *i;
      threads_.erase(i);
      return TryJoinThreads();
    }
  }
}

void BackgroundExecutor::JoinThreads() {
  for (std::vector<boost::thread *>::iterator i = threads_.begin(); i != threads_.end(); ++i) {
    (*i)->join();
    delete *i;
  }
  threads_.clear();
}

void DelayedExecutor::Start() {
  CHECK(thread_ == NULL);
  thread_ = new thread(bind(&DelayedExecutor::BackgroundThread, this));
}


DelayedExecutor::DelayedExecutor()
    : Executor(),
      stop_(false),
      thread_(NULL) {
  Start();
}

DelayedExecutor::~DelayedExecutor() {
  Stop();
  if (thread_)
    delete thread_;
}

void DelayedExecutor::Stop() {
  if (!thread_)
    return;
  stop_ = true;
  WakeBackgroundThread();
  bool ret = thread_->timed_join(boost::posix_time::seconds(1));
  if (!ret)
    thread_->detach();
}

void DelayedExecutor::WaitForAll() const {
  const auto &i = callbacks_.rbegin();
  if (i == callbacks_.rend())
    return;
  if (i->run_time > Timestamp::Now())
    boost::this_thread::sleep(boost::posix_time::millisec(i->run_time - Timestamp::Now() + 1));
}

void DelayedExecutor::Add(const Callback &callback) {
  Add(callback, Timestamp::Now());
}

void DelayedExecutor::Add(const Callback &callback, Timestamp when, Executor *executor) {
  if (when <= Timestamp::Now()) {
    // Run it now, there's no point adding it to the list.
    if (executor)
      executor->Add(callback);
    else
      callback();
    return;
  }
  TimedCallback cb;
  cb.callback = callback;
  cb.run_time = when;
  cb.executor = executor;
  callbacks_.insert(cb);
  WakeBackgroundThread();
}

void DelayedExecutor::WakeBackgroundThread() {
  CHECK(thread_);
  thread_->interrupt();
}

void DelayedExecutor::BackgroundThread() {
  while (!stop_) {
    uint64_t next_callback = 0;
    vector<TimedCallback> done_callbacks;
    for (auto &i : callbacks_) {
      if (i.run_time <= Timestamp::Now()) {
        boost::this_thread::disable_interruption di;
        if (i.executor) {
          // Run the callback in another thread
          i.executor->Add(i.callback);
        } else {
          // Run the callback in this thread
          try {
            i.callback();
          } catch (exception &e) {
            LOG(ERROR) << "DelayedExecutor callback threw exception: " << e.what();
          }
        }
        done_callbacks.push_back(i);
      } else if (!next_callback || i.run_time < next_callback) {
        next_callback = i.run_time;
      }
    }
    for (auto &i : done_callbacks) {
      callbacks_.erase(i);
    }
    uint64_t sleep_time = 100;
    if (next_callback)
      sleep_time = next_callback - Timestamp::Now();
    try {
      boost::this_thread::sleep(boost::posix_time::millisec(sleep_time));
    } catch (boost::thread_interrupted) {
      continue;
    }
  }
}

}  // namespace openinstrument
