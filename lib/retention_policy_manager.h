/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_RETENTION_POLICY_MANAGER_H_
#define _OPENINSTRUMENT_LIB_RETENTION_POLICY_MANAGER_H_

#include <boost/function.hpp>
#include <boost/timer.hpp>
#include "lib/common.h"
#include "lib/protobuf.h"

namespace openinstrument {

class Variable;

class RetentionPolicyManager {
 public:
  RetentionPolicyManager() {}

  const proto::RetentionPolicyItem &GetPolicy(const Variable &variable, uint64_t age) const;
  bool HasPolicyForVariable(const Variable &variable) const;
  bool ShouldDrop(const proto::RetentionPolicyItem &policy) const;
};

}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_RETENTION_POLICY_MANAGER_H_

