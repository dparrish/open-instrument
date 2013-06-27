/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <boost/function.hpp>
#include <boost/timer.hpp>
#include "lib/common.h"
#include "lib/protobuf.h"
#include "lib/variable.h"
#include "lib/retention_policy_manager.h"
#include "lib/store_config.h"

namespace openinstrument {

const proto::RetentionPolicyItem &RetentionPolicyManager::GetPolicy(const Variable &variable, uint64_t age) const {
  static proto::RetentionPolicyItem default_policy;
  default_policy.set_policy(proto::RetentionPolicyItem::DROP);
  VLOG(4) << "Looking for policy matches for " << variable.ToString() << " that is " << Duration(age)
          << " old";
  auto &config = StoreConfig::get();
  for (auto &item : config.retention_policy().policy()) {
    for (int j = 0; j < item.variable_size(); j++) {
      Variable match(item.variable(j));
      if (!variable.Matches(match)) {
        VLOG(4) << "  No match for " << match.ToString();
        continue;
      }
      if (item.min_age() && age < item.min_age()) {
        VLOG(4) << "  Skipping " << match.ToString() << ", var is too new (" << age << " < " << item.min_age() << ")";
        continue;
      }
      if (item.max_age() && age > item.max_age()) {
        VLOG(4) << "  Skipping " << match.ToString() << ", var is too old (" << age << " > " << item.max_age() << ")";
        continue;
      }
      VLOG(4) << "  Match for " << match.ToString() << " " << Duration(item.min_age()) << " < " << Duration(age)
              << " < " << Duration(item.max_age());
      return item;
    }
  }
  return default_policy;
}

bool RetentionPolicyManager::HasPolicyForVariable(const Variable &variable) const {
  auto &config = StoreConfig::get();
  for (auto &item : config.retention_policy().policy()) {
    for (int j = 0; j < item.variable_size(); j++) {
      Variable match(item.variable(j));
      if (variable.Matches(match))
        return true;
    }
  }
  return false;
}

bool RetentionPolicyManager::ShouldDrop(const proto::RetentionPolicyItem &policy) const {
  return policy.policy() == proto::RetentionPolicyItem::DROP;
}

}  // namespace openinstrument
