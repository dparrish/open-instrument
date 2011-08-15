/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_HOSTPORT_H_
#define _OPENINSTRUMENT_LIB_HOSTPORT_H_

#include <string>
#include "lib/common.h"
#include "lib/string.h"

namespace openinstrument {

using std::string;

void _empty_hostport_cache();

class HostPort {
 public:
  HostPort() : port_(0) {}
  HostPort(const string &hostname, int16_t port) : hostname_(hostname), port_(port) {}

  ~HostPort() {
    // Erase this object from the cache.
    // This will invalidate any existing references to this object, so be very careful.
    hostport_cache_.erase(format_key(hostname_, port_));
  }

  inline const string &hostname() const {
    return hostname_;
  }

  inline const int16_t port() const {
    return port_;
  }

  inline const string as_string() const {
    return format_key(hostname_, port_);
  }

  inline static string format_key(const string &hostname, int16_t port) {
    return StringPrintf("%s:%u", hostname.c_str(), port);
  }

  static HostPort *get(const string &hostname, int16_t port) {
    if (!done_atexit) {
      std::atexit(_empty_hostport_cache);
      done_atexit = true;
    }
    string keyname = format_key(hostname, port);
    if (hostport_cache_.find(keyname) == hostport_cache_.end()) {
      HostPort *hp = new HostPort(hostname, port);
      hostport_cache_[keyname] = hp;
      return hp;
    }
    return hostport_cache_[keyname];
  }

  static void empty_cache() {
    for (unordered_map<string, HostPort *>::iterator i = hostport_cache_.begin(); i != hostport_cache_.end(); ++i) {
      delete i->second;
    }
    hostport_cache_.empty();
  }

 protected:
  string hostname_;
  int16_t port_;

 private:
  static unordered_map<string, HostPort *> hostport_cache_;
  static bool done_atexit;
};

}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_HOSTPORT_H_
