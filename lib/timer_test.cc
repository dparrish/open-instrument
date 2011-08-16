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

TEST_F(TimerTest, DurationFromString) {
  EXPECT_EQ(0, Duration::FromString("").ms());  // invalid duration string
  EXPECT_EQ(0, Duration::FromString("1").ms());  // invalid duration string
  EXPECT_EQ(1000, Duration::FromString("1s").ms());
  EXPECT_FLOAT_EQ(11, Duration::FromString("10s1s").seconds());
  EXPECT_FLOAT_EQ(50, Duration::FromString("50s").seconds());
  EXPECT_FLOAT_EQ(120, Duration::FromString("2 m").seconds());
  EXPECT_FLOAT_EQ(7200, Duration::FromString("2h").seconds());
  EXPECT_FLOAT_EQ(86430, Duration::FromString("1d30s").seconds());
  EXPECT_FLOAT_EQ(172800, Duration::FromString("2d").seconds());
  EXPECT_FLOAT_EQ(1209600, Duration::FromString("2w").seconds());
  EXPECT_FLOAT_EQ(60480000, Duration::FromString("700d").seconds());
  EXPECT_FLOAT_EQ(63072000, Duration::FromString("2y").seconds());
  EXPECT_FLOAT_EQ(32746044, Duration::FromString("1y2w7m24s").seconds());
}

TEST_F(TimerTest, DurationToString) {
  EXPECT_EQ("1s", Duration::FromString("1s").ToString());
  EXPECT_EQ("11s", Duration::FromString("10s1s").ToString());
  EXPECT_EQ("50s", Duration::FromString("50s").ToString());
  EXPECT_EQ("2m", Duration::FromString("2 m").ToString());
  EXPECT_EQ("2h", Duration::FromString("2h").ToString());
  EXPECT_EQ("1d 30s", Duration::FromString("1d30s").ToString());
  EXPECT_EQ("2d", Duration::FromString("2d").ToString());
  EXPECT_EQ("2w", Duration::FromString("2w").ToString());
  EXPECT_EQ("1y 47w 6d", Duration::FromString("700d").ToString());
  EXPECT_EQ("1y", Duration::FromString("700d").ToString(false));
  EXPECT_EQ("2y", Duration::FromString("2y").ToString());
  EXPECT_EQ("1y 2w 7m 24s", Duration::FromString("1y2w7m24s").ToString());
  EXPECT_EQ("1y", Duration::FromString("1y2w7m24s").ToString(false));
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

