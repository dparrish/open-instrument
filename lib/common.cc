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

char *HexToBuffer(uint32_t i, char *buffer) {
  static const char *hexdigits = "0123456789abcdef";
  char *p = buffer + 21;
  // Write characters to the buffer starting at the end and work backwards.
  *p-- = '\0';
  do {
    *p-- = hexdigits[i & 15];   // mod by 16
    i >>= 4;                    // divide by 16
  } while (i > 0);
  return p + 1;
}

string HexToBuffer(uint32_t i) {
  char buf[21];
  char *p = HexToBuffer(i, buf);
  return p;
}

}  // namespace
