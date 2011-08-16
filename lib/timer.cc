/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <glog/logging.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <string>
#include "lib/common.h"
#include "lib/timer.h"

namespace openinstrument {

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
}  // namespace openinstrument
