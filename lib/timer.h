/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_TIMESTAMP_
#define _OPENINSTRUMENT_LIB_TIMESTAMP_

#include <boost/thread.hpp>
#include <glog/logging.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <string>
#include "lib/common.h"

namespace openinstrument {

using std::string;

// Returns ms since epoch given an input time string and format.
// Throws out_of_range on error
uint64_t strptime_64(const char *input, const char *format);

// Make a time_t (seconds since epoch) out a struct tm.
// This is a replacement for mktime that assumes GMT.
// Returns negative on failure.
time_t mkgmtime(const struct tm &tm);


class Timestamp {
 public:
  Timestamp() : ms_(Now()) {}
  Timestamp(const uint64_t ms) : ms_(ms) {}
  Timestamp(const Timestamp &ts) : ms_(ts.ms()) {}
  ~Timestamp() {}

  // Returns the timestamp value in milliseconds since epoch.
  inline uint64_t ms() const {
    return ms_;
  }

  // This is an approximation of seconds, and should not be used for any calculations. This is suitable for user
  // display.
  inline double seconds() const {
    return MsToSeconds(ms_);
  }

  // Returns the current time in milliseconds since epoch.
  inline static uint64_t Now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return static_cast<uint64_t>(tv.tv_sec) * 1000 + static_cast<uint64_t>(tv.tv_usec) / 1000;
  }

  inline static double MsToSeconds(uint64_t ms) {
    return static_cast<double>(ms / 1000.0);
  }

  inline void operator=(const Timestamp &other) {
    ms_ = other.ms();
  }

  inline void operator=(const double seconds) {
    ms_ = seconds * 1000;
  }

  inline void operator=(const uint64_t ms) {
    ms_ = ms;
  }

  inline void operator+(const Timestamp &other) {
    ms_ += other.ms();
  }

  inline void operator+(const uint64_t &other) {
    ms_ += other;
  }

  inline void operator-(const Timestamp &other) {
    ms_ -= other.ms();
  }

  inline void operator-(const uint64_t &other) {
    ms_ -= other;
  }

  // Renders the timestamp using strftime().
  // This also accepts another formatting conversion "%." which is replaced with the timestamp's fraction of a second,
  // which is useful in something like:
  //   Timestamp().strftime("%H:%M:%S.%.") --> 12:15:05.114
  string GmTime(const char *format = "%Y-%m-%d %H:%M:%S.%.") const;

  static uint64_t FromGmTime(const string &input, const char *format = "%Y-%m-%d %H:%M:%S");

 protected:
  uint64_t ms_;
};

// A basic high-resolution timer.
// This uses the gettimeofday(2) call which is a system call, and as such may cause a context switch which can skew the
// timings.
// The class provides timings in milliseconds, and is only accurate to around 30us because of the inherent system calls.
class Timer {
 public:
  Timer() : start_time_(0), elapsed_(0) {}

  // Start the timer. Be sure to call this before stop() or you will get a CHECK failure.
  inline void Start() {
    start_time_ = Timestamp::Now();
  }

  // Stop the counter and increase the elapsed time
  inline void Stop() {
    CHECK(start_time_) << ": Timer.Start() was not called";
    elapsed_ += Timestamp::Now() - start_time_;
  }

  // Return the current timer duration in milliseconds
  inline int64_t ms() const {
    CHECK(start_time_) << ": Timer.Start() was not called";
    return elapsed_;
  }

  // This is an approximation of seconds, and should not be used for any calculations. This is suitable for user
  // display.
  inline double seconds() const {
    return Timestamp::MsToSeconds(elapsed_);
  }

 protected:
  int64_t start_time_;
  int64_t elapsed_;
};


// Use this class to automatically start an existing timer and stop it at scope exit.
//
// Usage:
// Timer timer;
// { // Sample scope
//   ScopedTimer t(&timer);
//   // do something here
// }
// // At this point timer.stop() has been called and you can get the duration of the inner scope with timer.ms()
class ScopedTimer {
 public:
  explicit ScopedTimer(Timer *timer) : timer_(timer) {
    timer_->Start();
  }

  ~ScopedTimer() {
    timer_->Stop();
  }

 private:
  Timer *timer_;
};


class Duration : public Timestamp {
 public:
  Duration() {}
  explicit Duration(int64_t ms) : Timestamp(ms) {}
  explicit Duration(const string &str) : Timestamp(FromString(str).ms()) {}
  ~Duration() {}

  // Parse a string containing a duration and return the number of milliseconds.
  // Understands formats like:
  //   121s  - 121 seconds
  //   1m    - 1 minute
  //   1d    - 1 day
  //   1w    - 1 week
  //   1m    - 1 month
  //   1y    - 1 year
  //   1y1m1w1d1h1s = 1 year, 1 month, 1 week, 1 day, 1 hour, 1 second
  static Duration FromString(const string &duration);

  string ToString(bool longformat = true);
};

class Deadline {
 public:
  explicit Deadline(uint64_t deadline) : deadline_(deadline), start_(Timestamp::Now()) {}

  inline operator uint64_t() const {
    int64_t timeleft = deadline_ - (Timestamp::Now() - start_);
    return timeleft < 0 ? 0 : timeleft;
  }

  inline void Reset() {
    start_ = Timestamp::Now();
  }

 private:
  uint64_t deadline_;
  uint64_t start_;
};

// High-resolution timer that keeps track of the amount of CPU time consumed by the current PROCESS.
// This will likely not relate to real time unless the process is using 100% CPU.
class ProcessCpuTimer {
 public:
  explicit ProcessCpuTimer(clockid_t clock = CLOCK_PROCESS_CPUTIME_ID) : clock_(clock), running_(false) {}

  void Start() {
    CHECK(!running_);
    tid_ = boost::this_thread::get_id();
    clock_gettime(clock_, &start_ts_);
    running_ = true;
  }

  void Stop() {
    CHECK(running_);
    CHECK(boost::this_thread::get_id() == tid_);
    clock_gettime(clock_, &end_ts_);
    running_ = false;
  }

  int64_t ms();
  int64_t us();

 protected:
  const clockid_t clock_;

 private:
  bool running_;
  boost::thread::id tid_;
  struct timespec start_ts_;
  struct timespec end_ts_;
};

// High-resolution timer that keeps track of the amount of CPU time consumed by the current THREAD.
// This will likely not relate to real time unless the thread is using 100% CPU.
class ThreadCpuTimer : public ProcessCpuTimer {
 public:
  ThreadCpuTimer() : ProcessCpuTimer(CLOCK_THREAD_CPUTIME_ID) {}
};

}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_TIMESTAMP__
