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

#include <glog/logging.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <string>
#include "lib/common.h"

namespace openinstrument {

using std::string;

class Timestamp {
 public:
  Timestamp() : ms_(Now()) {}
  Timestamp(const int64_t ms) : ms_(ms) {}
  Timestamp(const Timestamp &ts) : ms_(ts.ms()) {}
  ~Timestamp() {}

  // Returns the timestamp value in milliseconds since epoch.
  inline int64_t ms() const {
    return ms_;
  }

  // This is an approximation of seconds, and should not be used for any calculations. This is suitable for user
  // display.
  inline double seconds() const {
    return MsToSeconds(ms_);
  }

  // Returns the current time in milliseconds since epoch.
  inline static int64_t Now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + tv.tv_usec / 1000;
  }

  inline static double MsToSeconds(int64_t ms) {
    return static_cast<double>(ms / 1000.0);
  }

  void operator=(const Timestamp &other) {
    ms_ = other.ms();
  }

  void operator=(const double seconds) {
    ms_ = seconds * 1000;
  }

  void operator=(const uint64_t ms) {
    ms_ = ms;
  }

  void operator+(const Timestamp &other) {
    ms_ += other.ms();
  }

  void operator+(const int64_t &other) {
    ms_ += other;
  }

  void operator-(const Timestamp &other) {
    ms_ -= other.ms();
  }

  void operator-(const int64_t &other) {
    ms_ -= other;
  }

  // Renders the timestamp using strftime().
  // This also accepts another formatting conversion "%." which is replaced with the timestamp's fraction of a second,
  // which is useful in something like:
  //   Timestamp().strftime("%H:%M:%S.%.") --> 12:15:05.114
  string GmTime(const char *format) const {
    time_t sec = seconds();
    struct tm tm;
    if (!gmtime_r(&sec, &tm))
      return "";
    char buf[128] = {0};
    if (strftime(buf, 127, format, &tm) <= 0)
      return "";
    string out(buf);
    size_t pos = out.find("%.");
    if (pos != string::npos)
      out.replace(pos, 2, lexical_cast<string>(ms() % 1000));
    return out;
  }

 protected:
  int64_t ms_;
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
  static Duration Parse(const string &duration) {
    char *newstr = strndup(duration.data(), duration.size());
    char *start = newstr;
    char *p = newstr;
    int64_t seconds = 0;
    for (p = start; p < newstr + duration.size(); ++p) {
      if ((*p >= '0' && *p <= '9') || *p == ' ')
        continue;
      char multiplier = *p;
      *p = '\x00';
      switch (multiplier) {
        case 's':  // seconds
          seconds += atoll(start);
          break;
        case 'm':  // minutes
          seconds += atoll(start) * 60LL;
          break;
        case 'h':  // hours
          seconds += atoll(start) * 3600LL;
          break;
        case 'd':  // days
          seconds += atoll(start) * 86400LL;
          break;
        case 'w':  // weeks
          seconds += atoll(start) * 604800LL;
          break;
        case 'y':  // years
          seconds += atoll(start) * 31556926LL;
          break;
        default:
          LOG(WARNING) << "Unknown duration specification '" << start << "'";
          break;
      }
      start = p + 1;
    }
    free(newstr);

    return Duration(seconds * 1000);
  }
};

class Deadline {
 public:
  explicit Deadline(uint64_t deadline) : deadline_(deadline), start_(Timestamp::Now()) {}

  operator uint64_t() const {
    int64_t timeleft = deadline_ - (Timestamp::Now() - start_);
    return timeleft < 0 ? 0 : timeleft;
  }

  void Reset() {
    start_ = Timestamp::Now();
  }

 private:
  uint64_t deadline_;
  uint64_t start_;
};

}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_TIMESTAMP__
