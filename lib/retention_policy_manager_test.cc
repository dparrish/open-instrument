/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <string>
#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include "lib/common.h"
#include "lib/retention_policy_manager.h"
#include "lib/string.h"
#include "lib/store_config.h"
#include "lib/variable.h"

DECLARE_string(datastore);

namespace openinstrument {

class RetentionPolicyManagerTest : public ::testing::Test {};

TEST_F(RetentionPolicyManagerTest, DefaultPolicy) {
  StoreConfig::get_manager().SetConfigFilename("../config.txt");
  RetentionPolicyManager manager;
  {
    auto &policy = manager.GetPolicy(Variable("/openinstrument/test/test1"), 1);
    EXPECT_EQ(1, policy.policy());
    EXPECT_EQ(0, policy.mutation_size());
  }

  {
    auto policy = manager.GetPolicy(Variable("/openinstrument/test/test1"), 2419200001);
    EXPECT_EQ(1, policy.policy());
    EXPECT_EQ(1, policy.mutation_size());
    EXPECT_EQ(1, policy.mutation(0).sample_type());
  }

  {
    auto policy = manager.GetPolicy(Variable("/openinstrument/test/test1{retain=forever}"), 2419200001);
    EXPECT_EQ(1, policy.policy());
    EXPECT_EQ(0, policy.mutation_size());
  }

  {
    auto policy = manager.GetPolicy(Variable("/openinstrument/test/test1"), 86400LL * 1000 * 365 * 6);
    EXPECT_EQ(2, policy.policy());
    EXPECT_EQ(0, policy.mutation_size());
  }

  {
    auto policy = manager.GetPolicy(Variable("/openinstrument/test/test1{retain=forever}"), 86400LL * 1000 * 365 * 6);
    EXPECT_EQ(1, policy.policy());
    EXPECT_EQ(0, policy.mutation_size());
  }
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
