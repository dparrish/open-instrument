/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_CONSISTENT_HASH_H_
#define _OPENINSTRUMENT_LIB_CONSISTENT_HASH_H_

#include <boost/functional/hash.hpp>
#include "lib/common.h"

namespace openinstrument {

class Hash {
 public:
  // Returns a 32 bit hash value for a string
  static uint32_t Hash32(const string &str, uint32_t initval = 0);

  // Returns a 32 bit hash value for a list of strings
  static uint32_t Hash32(const vector<string> &str, uint32_t initval = 0);

  // Returns a 64 bit hash value for a string
  static uint64_t Hash64(const string &str, uint64_t initval = 0);

  // Returns two hashes for a single string.
  static void DoubleHash32(const string &str, uint32_t *pc, uint32_t *pb);
};

template <class Node=string, class Data=string>
class HashRing {
 public:
  typedef std::map<uint32_t, Node> NodeMap;

  // Create a ring with <replicas> number of replicas of each node.
  HashRing(unsigned int replicas) : replicas_(replicas) {}

  // Add a node to the ring.
  // This generally corresponds to a server or destination.
  void AddNode(const Node &node) {
    for (unsigned int r = 0; r < replicas_; r++) {
      uint32_t hash = Hash::Hash32(StringPrintf("%s%u", lexical_cast<string>(node).c_str(), r));
      ring_[hash] = node;
    }
  }

  void Clear() {
    ring_.clear();
  }

  void RemoveNode(const Node &node) {
    for (unsigned int r = 0; r < replicas_; r++) {
      uint32_t hash = Hash::Hash32(StringPrintf("%s%s", lexical_cast<string>(node).c_str(), r));
      ring_.erase(hash);
    }
  }

  // Get appropriate node for <data>.
  const Node &GetNode(const Data &data) const {
    if (ring_.empty())
      throw std::out_of_range("Empty ring in consistent hash");
    uint32_t hash = Hash::Hash32(data);
    typename NodeMap::const_iterator it;
    // Look for the first node >= hash
    it = ring_.lower_bound(hash);
    if (it == ring_.end()) {
      // Wrapped around; get the first node
      it = ring_.begin();
    }
    return it->second;
  }

  const Node &GetBackupNode(const Data &data) const {
    if (ring_.empty())
      throw std::out_of_range("Empty ring in consistent hash");
    uint32_t hash = Hash::Hash32(data);
    typename NodeMap::const_iterator it;
    // Look for the first node >= hash
    it = ring_.lower_bound(hash);
    if (it == ring_.end())
      it = ring_.begin();
    ++it;
    if (it == ring_.end())
      it = ring_.begin();
    return it->second;
  }

 private:
  NodeMap ring_;
  const unsigned int replicas_;
};


}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_CONSISTENT_HASH_H_
