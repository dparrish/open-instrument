/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_COMMON_H_
#define _OPENINSTRUMENT_LIB_COMMON_H_

#include <boost/algorithm/string/predicate.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/noncopyable.hpp>
#include <boost/regex.hpp>
#include <boost/scoped_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/timer.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/unordered_map.hpp>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <google/protobuf/message.h>
#include <climits>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace openinstrument {

using boost::array;
using boost::bind;
using boost::enable_shared_from_this;
using boost::indeterminate;
using boost::lexical_cast;
using boost::make_tuple;
using boost::noncopyable;
using boost::scoped_array;
using boost::shared_ptr;
using boost::thread;
using boost::tie;
using boost::tribool;
using boost::tuple;
using boost::unordered_map;
using std::list;
using std::set;
using std::size_t;
using std::string;
using std::vector;

// Common exceptions
using std::runtime_error;
using std::out_of_range;
using std::exception;

typedef boost::mutex Mutex;
typedef boost::unique_lock<Mutex> MutexLock;

typedef boost::shared_mutex SharedMutex;
typedef boost::unique_lock<SharedMutex> ExclusiveLock;
typedef boost::shared_lock<SharedMutex> SharedLock;
typedef boost::mutex::scoped_lock ScopedLock;

// Wrapper around boost::this_thread::sleep, so that threads sleeping become interruptable
void sleep(uint64_t interval);

template<class T>
class scoped_ptr : private noncopyable {
 public:
  typedef T element_type;

  explicit scoped_ptr(T *p = 0) : ptr_(p) {}
  ~scoped_ptr() {
    if (ptr_)
      delete ptr_;
  }

  inline void reset(T *p = 0) {
    if (ptr_)
      delete ptr_;
    ptr_ = p;
  }

  inline T &operator*() const {
    return *ptr_;
  }

  inline T *operator->() const {
    return ptr_;
  }

  inline T *release() {
    T *p = ptr_;
    ptr_ = NULL;
    return p;
  }

  inline T *get() const {
    return ptr_;
  }

  inline void swap(scoped_ptr &b) {
    T *p = b.ptr_;
    b.ptr_ = ptr_;
    ptr_ = p;
  }

 protected:
  scoped_ptr() {}
  T *ptr_;
};

template<typename T>
class Queue
{
 public:
  Queue() : shutdown_(false) {}

  ~Queue() {
    Shutdown();
  }

  void Shutdown() {
    shutdown_ = true;
    condvar_.notify_all();
  }

  bool empty() const {
    ScopedLock lock(mutex_);
    return queue_.empty();
  }

  void Push(const T &data) {
    if (shutdown_)
      throw std::out_of_range("Queue shut down");
    ScopedLock lock(mutex_);
    queue_.push(data);
    lock.unlock();
    condvar_.notify_one();
  }

  bool TryPop(T &popped_value) {
    if (shutdown_)
      throw std::out_of_range("Queue shut down");
    ScopedLock lock(mutex_);
    if (!shutdown_ && queue_.empty()) {
      return false;
    }
    if (shutdown_)
      throw std::out_of_range("Queue shut down");

    popped_value = queue_.front();
    queue_.pop();
    return true;
  }

  void WaitAndPop(T &popped_value) {
    if (shutdown_)
      throw std::out_of_range("Queue shut down");
    ScopedLock lock(mutex_);
    while (!shutdown_ && queue_.empty()) {
      condvar_.wait(lock);
    }
    if (shutdown_)
      throw std::out_of_range("Queue shut down");

    popped_value = queue_.front();
    queue_.pop();
  }

 private:
  bool shutdown_;
  std::queue<T> queue_;
  mutable boost::mutex mutex_;
  boost::condition_variable condvar_;
};

class Notification : private noncopyable {
 public:
  Notification() : ready_(false) {}
  void Notify();
  bool WaitForNotification() const;
  bool WaitForNotification(uint64_t timeout) const;
  inline bool HasBeenNotified() const {
    return ready_;
  }

 private:
  bool ready_;
  mutable boost::condition_variable condvar_;
  mutable Mutex mutex_;
};

class Refcountable {
 public:
  Refcountable() : refcount_() {}
  ~Refcountable() {}

  virtual void RefcountDestroy() {
    delete this;
  };

  int ref() {
    return ++refcount_;
  }
  int deref() {
    if (--refcount_ <= 0)
      RefcountDestroy();
    return refcount_;
  }

 protected:
  int refcount() const {
    return refcount_;
  }

 private:
  mutable int refcount_;
};

template<class T>
class refcount_ptr : private noncopyable {
 public:
  typedef T element_type;

  explicit refcount_ptr(T *p = 0) : ptr_(p) {
    if (ptr_)
      ptr_->ref();
  }
  ~refcount_ptr() {
    if (ptr_)
      ptr_->deref();
  }

  inline void reset(T *p = 0) {
    if (ptr_)
      ptr_->deref();
    ptr_ = p;
  }

  inline T &operator*() const {
    return *ptr_;
  }

  inline T *operator->() const {
    return ptr_;
  }

  inline T *release() {
    T *p = ptr_;
    ptr_ = NULL;
    return p;
  }

  inline T *get() const {
    return ptr_;
  }

  inline void swap(refcount_ptr &b) {
    T *p = b.ptr_;
    b.ptr_ = ptr_;
    ptr_ = p;
  }

 protected:
  refcount_ptr() {}
  T *ptr_;
};

// Write the supplied uint32_t to a buffer as a hex string.
// The buffer must be supplied pre-allocated with 21 bytes.
// Returns a pointer to the first character in the hex string.
char *HexToBuffer(uint32_t i, char *buffer);
string HexToBuffer(uint32_t i);

}  // namespace

#include "lib/closure.h"
#include "lib/string.h"

#endif  // _OPENINSTRUMENT_LIB_COMMON_H_
