/*
 *  -
 *
 * (c) 2010 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include "server/hostport.h"

namespace openinstrument {

unordered_map<string, HostPort *> HostPort::hostport_cache_;
bool HostPort::done_atexit = false;

void _empty_hostport_cache() {
  HostPort::empty_cache();
}

}  // namespace
