/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <gtest/gtest.h>
#include <string>
#include "lib/timer.h"

namespace openinstrument {

class TimerTest : public ::testing::Test {};

TEST_F(TimerTest, ParseDurations) {
  EXPECT_EQ(0, Duration::Parse("").ms());  // invalid duration string
  EXPECT_EQ(0, Duration::Parse("1").ms());  // invalid duration string
  EXPECT_EQ(1000, Duration::Parse("1s").ms());
  EXPECT_EQ(11, Duration::Parse("10s1s").seconds());
  EXPECT_EQ(50, Duration::Parse("50s").seconds());
  EXPECT_EQ(120, Duration::Parse("2 m").seconds());
  EXPECT_EQ(7200, Duration::Parse("2h").seconds());
  EXPECT_EQ(86430, Duration::Parse("1d30s").seconds());
  EXPECT_EQ(172800, Duration::Parse("2d").seconds());
  EXPECT_EQ(1209600, Duration::Parse("2w").seconds());
  EXPECT_EQ(60480000, Duration::Parse("700d").seconds());
  EXPECT_EQ(63113852, Duration::Parse("2y").seconds());
  EXPECT_EQ(32766970, Duration::Parse("1y2w7m24s").seconds());
}

TEST_F(TimerTest, Strftime) {
  Timestamp t(1294356842114);
  EXPECT_EQ("2011-01-06 23:34:02.114 GMT", t.GmTime("%Y-%m-%d %H:%M:%S.%. %Z"));
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

