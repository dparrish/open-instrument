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
#include <stdexcept>
#include <string>
#include <vector>
#include "lib/closure.h"
#include "lib/string.h"
#include "lib/threadpool.h"

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
using std::size_t;
using std::string;
using std::vector;

// Common exceptions
using std::runtime_error;
using std::out_of_range;
using std::exception;

typedef boost::mutex Mutex;
typedef boost::unique_lock<Mutex> MutexLock;

template<class T>
class scoped_ptr : noncopyable {
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
class Queue {
 public:
  void push(T *data) {
    MutexLock lock(mutex_);
    queue_.push(data);
    lock.unlock();
    condvar_.notify_one();
  }

  bool empty() const {
    MutexLock lock(mutex_);
    return queue_.empty();
  }

  T *try_pop() {
    MutexLock lock(mutex_);
    if (queue_.empty())
      return NULL;

    T *popped_value = queue_.front();
    queue_.pop();
    return popped_value;
  }

  T *wait_and_pop() {
    MutexLock lock(mutex_);
    while (queue_.empty())
      condvar_.wait(lock);

    T *popped_value = queue_.front();
    queue_.pop();
    return popped_value;
  }

 private:
  std::queue<T *> queue_;
  mutable MutexLock mutex_;
  boost::condition_variable condvar_;
};


template <class T>
class Buffer {
 public:
  Buffer() : size_(0), capacity_(0) {}

  explicit Buffer(int capacity) {
    reserve(capacity);
  }

  void clear() {
    size_ = 0;
    capacity_ = 0;
    t_.Reset();
  }

  void reserve(int capacity) {
    size_ = 0;
    t_.Reset(new T[capacity + 1]);
    capacity_ = capacity;
  }

  T *data() const {
    return t_.get() + size_;
  }

  T *content() const {
    return t_.get();
  }

  void inc_size(int size) {
    size_ += size;
  }

  void push_back(const T &t) {
    *data() = t;
    inc_size(1);
  }

  int size() const {
    return capacity_ - size_;
  }

  bool full() const {
    return size() == 0;
  }

  int capacity() const {
    return capacity_;
  }

 private:
  scoped_array<T> t_;
  int size_;
  int capacity_;
};

}  // namespace

#endif  // _OPENINSTRUMENT_LIB_COMMON_H_
