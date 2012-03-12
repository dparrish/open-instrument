/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#define _XOPEN_SOURCE

#include <boost/date_time/posix_time/posix_time.hpp>
#include <glog/logging.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <string>
#include "lib/common.h"
#include "lib/timer.h"

namespace openinstrument {

time_t mkgmtime(const struct tm &tm) {
  static const int month_day[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  time_t tm_year = tm.tm_year;
  time_t tm_mon = tm.tm_mon;
  time_t tm_mday = tm.tm_mday;
  time_t tm_hour = tm.tm_hour;
  time_t tm_min = tm.tm_min;
  time_t tm_sec = tm.tm_sec;
  time_t month = tm_mon % 12;
  time_t year = tm_year + tm_mon / 12;
  if (month < 0) {
    month += 12;
    --year;
  }
  const time_t year_for_leap = (month > 1) ? year + 1 : year;
  return tm_sec + 60 * (tm_min + 60 * (tm_hour + 24 * (month_day[month] + tm_mday - 1 + 365 * (year - 70)
       + (year_for_leap - 69) / 4 - (year_for_leap - 1) / 100 + (year_for_leap + 299) / 400)));
}

uint64_t strptime_64(const char *input, const char *format) {
  if (!input || !strlen(input))
    throw out_of_range("No input string");

  uint32_t ms = 0;
  struct tm tm = {0, };
  const char *p = strptime(input, format, &tm);
  if (p == NULL)
    throw out_of_range("Invalid input string for format");
  if (*p == '.') {
    // There are milliseconds
    if (strlen(p) < 2)
      throw out_of_range("Not enough characters left in input for ms");

    if (sscanf(p, ".%3d", &ms) != 1)
      throw out_of_range("Milliseconds not found in input string");
    p += 4;
  }
  uint64_t ret = mkgmtime(tm);
  if (ret < 0)
    throw out_of_range("Invalid return from mkgmtime");
  return ret * 1000 + ms;
}

string Timestamp::GmTime(const char *format) const {
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

uint64_t Timestamp::FromGmTime(const string &input, const char *format) {
  return strptime_64(input.c_str(), format);
}

Duration Duration::FromString(const string &duration) {
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
        seconds += atoll(start) * (60 * 60 * 24 * 7);
        break;
      case 'y':  // years
        seconds += atoll(start) * (60 * 60 * 24 * 365);
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

string Duration::ToString(bool longformat) {
  string output;
  if (ms_ < 1000)
    return StringPrintf("%llums", ms_);
  uint64_t s = ms() / 1000;
  while (s > 0) {
    if (s > 60 * 60 * 24 * 365) {
      StringAppendf(&output, "%lluy ", s / (60 * 60 * 24 * 365));
      s %= (60 * 60 * 24 * 365);
    } else if (s >= 60 * 60 * 24 * 7) {
      StringAppendf(&output, "%lluw ", s / (60 * 60 * 24 * 7));
      s %= (60 * 60 * 24 * 7);
    } else if (s >= 60 * 60 * 24) {
      StringAppendf(&output, "%llud ", s / (60 * 60 * 24));
      s %= (60 * 60 * 24);
    } else if (s >= 60 * 60) {
      StringAppendf(&output, "%lluh ", s / (60 * 60));
      s %= (60 * 60);
    } else if (s >= 60) {
      StringAppendf(&output, "%llum ", s / 60);
      s %= 60;
    } else if (s > 0) {
      StringAppendf(&output, "%llus ", s);
      break;
    }
    if (!longformat)
      break;
  }
  if (output.size())
    return output.substr(0, output.size() - 1);
  return output;
}

int64_t ProcessCpuTimer::ms() {
  if (running_)
    clock_gettime(clock_, &end_ts_);
  return ((static_cast<int64_t>(end_ts_.tv_sec - start_ts_.tv_sec) * 1000) +
          (static_cast<int64_t>(end_ts_.tv_nsec - start_ts_.tv_nsec)) / 1000000);
}

int64_t ProcessCpuTimer::us() {
  if (running_) {
    CHECK(boost::this_thread::get_id() == tid_);
    clock_gettime(clock_, &end_ts_);
  }
  return ((static_cast<int64_t>(end_ts_.tv_sec - start_ts_.tv_sec) * 1000000) +
          (static_cast<int64_t>(end_ts_.tv_nsec - start_ts_.tv_nsec) / 1000));
}

}  // namespace openinstrument
