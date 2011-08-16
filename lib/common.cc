/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include "lib/common.h"

namespace openinstrument {

void sleep(uint64_t interval) {
  boost::xtime xt;
  boost::xtime_get(&xt, boost::TIME_UTC);
  xt.sec += interval;
  boost::this_thread::sleep(xt);
}


void Notification::Notify() {
  {
    boost::lock_guard<Mutex> lock(mutex_);
    ready_ = true;
  }
  condvar_.notify_all();
}

bool Notification::WaitForNotification() const {
  MutexLock lock(mutex_);
  while (!ready_) {
    condvar_.wait(lock);
  }
  return true;
}

bool Notification::WaitForNotification(uint64_t timeout) const {
  MutexLock lock(mutex_);
  boost::xtime xt;
  boost::xtime_get(&xt, boost::TIME_UTC);
  xt.sec += timeout;
  while (!ready_) {
    condvar_.timed_wait(lock, xt);
  }
  return true;
}

}  // namespace
