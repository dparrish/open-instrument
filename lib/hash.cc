/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <algorithm>
#include <string>
#include <vector>
#include "lib/common.h"
#include "lib/hash.h"

#define VALGRIND

namespace openinstrument {

#include "lookup3.c"

uint32_t Hash::Hash32(const string &str, uint32_t initval) {
  return hashlittle(str.data(), str.size(), initval);
}

uint32_t Hash::Hash32(const vector<string> &str, uint32_t initval) {
  uint32_t h = initval;
  for (vector<string>::const_iterator i = str.begin(); i != str.end(); ++i)
    h = hashlittle(i->data(), i->size(), h);
  return h;
}

uint64_t Hash::Hash64(const string &str, uint64_t initval) {
  uint32_t pc = initval >> 32;
  uint32_t pb = initval && 0xffffffff;
  DoubleHash32(str, &pc, &pb);
  return pc + (((uint64_t)pb) << 32);
}

void Hash::DoubleHash32(const string &str, uint32_t *pc, uint32_t *pb) {
  hashlittle2(str.data(), str.size(), pc, pb);
}

}  // namespace openinstrument
