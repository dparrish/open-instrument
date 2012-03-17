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
#include "lib/retention_policy.h"
#include "lib/string.h"
#include "server/store_config.h"

namespace openinstrument {

class RetentionPolicyTest : public ::testing::Test {};

TEST_F(RetentionPolicyTest, ShouldDrop) {
  StoreConfig config("../config.txt");
  config.WaitForConfigLoad();

  RetentionPolicy policy(&config);
  EXPECT_FALSE(policy.ShouldDrop(policy.GetPolicy(Variable("/junk/var"), Duration("1d"))));
  EXPECT_FALSE(policy.ShouldDrop(policy.GetPolicy(Variable("/junk/var{var=foo,retain=forever}"), Duration("5d"))));
  EXPECT_FALSE(policy.ShouldDrop(policy.GetPolicy(Variable("/junk/var{var=foo,retain=forever}"), Duration("1y"))));
  EXPECT_FALSE(policy.ShouldDrop(policy.GetPolicy(Variable("/junk/var{var=foo,retain=forever}"), Duration("100y"))));
  EXPECT_FALSE(policy.ShouldDrop(policy.GetPolicy(Variable("/junk/var"), Duration("5d"))));
  EXPECT_FALSE(policy.ShouldDrop(policy.GetPolicy(Variable("/junk/var"), Duration("1y"))));
  EXPECT_TRUE(policy.ShouldDrop(policy.GetPolicy(Variable("/junk/var"), Duration("100y"))));
  EXPECT_TRUE(policy.ShouldDrop(policy.GetPolicy(Variable("/openinstrument/process/cpuset"), Duration("1d"))));

  EXPECT_EQ(0, policy.GetPolicy(Variable("/junk/var"), Duration("1d")).mutation_size());
  EXPECT_EQ(1, policy.GetPolicy(Variable("/junk/var"), Duration("2y")).mutation_size());
  EXPECT_EQ(0, policy.GetPolicy(Variable("/junk/var"), Duration("100y")).mutation_size());
  const proto::StreamMutation &mutation = policy.GetPolicy(Variable("/junk/var{var=foo}"), Duration("2y")).mutation(0);
  EXPECT_EQ(proto::StreamMutation::AVERAGE, mutation.sample_type());
  EXPECT_EQ(Duration("1h"), mutation.sample_frequency());
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


