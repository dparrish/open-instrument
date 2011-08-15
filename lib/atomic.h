/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_ATOMIC_
#define _OPENINSTRUMENT_LIB_ATOMIC_

#include "lib/common.h"

namespace openinstrument {

// Counter variable that is updated atomically.
// A custom class is used instead of the GCC macros or equivalent assembler calls because they only work with 32bit
// ints, this class does the same with a 64 bit int.
class AtomicInt64 {
 public:
  explicit AtomicInt64(int64_t v) : value_(v) {}

  void operator=(const int64_t v) {
    MutexLock lock(mutex_);
    value_ = v;
  }

  int64_t operator++() {
    MutexLock lock(mutex_);
    return ++value_;
  }

  int64_t operator+=(const int64_t v) {
    MutexLock lock(mutex_);
    return value_ += v;
  }

  int64_t operator--() {
    MutexLock lock(mutex_);
    return --value_;
  }

  int64_t operator-=(const int64_t v) {
    MutexLock lock(mutex_);
    return value_ -= v;
  }

  operator int64_t() const {
    MutexLock lock(mutex_);
    return value_;
  }

 private:
  explicit AtomicInt64(AtomicInt64 const &);
  AtomicInt64 & operator=(AtomicInt64 const &);

  mutable Mutex mutex_;
  int64_t value_;
};

}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_ATOMIC_
