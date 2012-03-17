/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_RETENTION_POLICY_H_
#define _OPENINSTRUMENT_LIB_RETENTION_POLICY_H_

#include <boost/function.hpp>
#include <boost/timer.hpp>
#include "lib/common.h"
#include "server/store_config.h"

namespace openinstrument {

class RetentionPolicy {
 public:
  RetentionPolicy(StoreConfig *config)
    : config_(config) {
  }

  bool ValidateRetentionPolicy() {
    return true;
  }

  const proto::RetentionPolicyItem &GetPolicy(const Variable &variable, uint64_t age) const {
    static proto::RetentionPolicyItem default_policy;
    default_policy.set_policy(proto::RetentionPolicyItem::DROP);
    VLOG(4) << "Looking for policy matches for " << variable.ToString() << " that is " << Duration(age).ToString()
            << " old";
    for (int i = 0; i < config_->config().retention_policy().policy_size(); i++) {
      const proto::RetentionPolicyItem &item = config_->config().retention_policy().policy(i);
      Variable match(item.variable());
      if (!variable.Matches(match)) {
        VLOG(3) << "  No match for " << match.ToString();
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
      VLOG(4) << "  Match for " << match.ToString() << " " << Duration(item.min_age()).ToString() << " < "
              << Duration(age).ToString() << " < " << Duration(item.max_age()).ToString();
      return item;
    }
    return default_policy;
  }

  bool ShouldDrop(const proto::RetentionPolicyItem &policy) const {
    return policy.policy() == proto::RetentionPolicyItem::DROP;
  }

 private:
  StoreConfig *config_;
};

}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_COMMON_H_

