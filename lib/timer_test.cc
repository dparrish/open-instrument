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
#include "lib/common.h"

namespace openinstrument {

class TimerTest : public ::testing::Test {};

int callbacks_run = 0;
void PrintHello(int i) {
  callbacks_run++;
}

TEST_F(TimerTest, DelayedExecutor) {
  Executor exec1, exec2;
  DelayedExecutor executor;
  int i = 0;
  executor.Add(bind(PrintHello, ++i), Timestamp::Now() + 100);
  executor.Add(bind(PrintHello, ++i), Timestamp::Now() + 100, &exec1);
  executor.Add(bind(PrintHello, ++i), Timestamp::Now() + 350, &exec2);
  executor.Add(bind(PrintHello, ++i), Timestamp::Now() + 250, &exec1);
  executor.Add(bind(PrintHello, ++i), Timestamp::Now() + 200, &exec2);
  executor.Add(bind(PrintHello, ++i), Timestamp::Now() + 50, &exec2);

  executor.WaitForAll();
  EXPECT_EQ(callbacks_run, 6);
}

TEST_F(TimerTest, DurationFromString) {
  EXPECT_EQ(0ULL, Duration::FromString("").ms());  // invalid duration string
  EXPECT_EQ(0ULL, Duration::FromString("1").ms());  // invalid duration string
  EXPECT_EQ(1000ULL, Duration::FromString("1s").ms());
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
  string timestr = t.GmTime("%Y-%m-%d %H:%M:%S.%. %Z");
  EXPECT_EQ("2011-01-06 23:34:02.114 GMT", timestr);
  // TODO(dparrish): Add in tests for timezones
  EXPECT_EQ(1294356842114ULL, Timestamp::FromGmTime(timestr, "%Y-%m-%d %H:%M:%S"));
}

TEST_F(TimerTest, ProcessCpuTimer) {
  ProcessCpuTimer processtimer;
  Timer basictimer;
  processtimer.Start();
  basictimer.Start();
  // Count, just to consume some CPU time
  for (uint64_t i = 0; i < 500000000; ++i);
  processtimer.Stop();
  basictimer.Stop();
  EXPECT_NEAR(processtimer.ms(), static_cast<double>(processtimer.us()) / 1000, 10);
  VLOG(1) << "Process Timer: " << processtimer.ms() << " ms";
  VLOG(1) << "Process Timer: " << processtimer.us() << " us";
  VLOG(1) << "Basic Timer: " << basictimer.ms() << " ms";
}

TEST_F(TimerTest, ThreadCpuTimer) {
  ThreadCpuTimer threadtimer;
  Timer basictimer;
  threadtimer.Start();
  basictimer.Start();
  // Count, just to consume some CPU time
  for (uint64_t i = 0; i < 500000000; ++i);
  threadtimer.Stop();
  basictimer.Stop();
  EXPECT_NEAR(threadtimer.ms(), static_cast<double>(threadtimer.us()) / 1000, 10);
  VLOG(1) << "Thread Timer: " << threadtimer.ms() << " ms";
  VLOG(1) << "Thread Timer: " << threadtimer.us() << " us";
  VLOG(1) << "Basic Timer: " << basictimer.ms() << " ms";
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

