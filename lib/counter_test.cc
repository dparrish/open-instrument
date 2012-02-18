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
#include "lib/counter.h"
#include "lib/string.h"

namespace openinstrument {

class CounterTest : public ::testing::Test {};

class TimestampValue {
 public:
  TimestampValue(uint64_t timestamp, double value) : timestamp(timestamp), value(value) {}
  uint64_t timestamp;
  double value;
};

TEST_F(CounterTest, UniformTimeSeries) {
  // Pass a set of points in with non-uniform timestamps
  vector<TimestampValue> input;
  input.push_back(TimestampValue(0, 10));
  input.push_back(TimestampValue(1, 10));
  input.push_back(TimestampValue(30, 60));
  input.push_back(TimestampValue(41, 70));
  input.push_back(TimestampValue(70, 130));
  input.push_back(TimestampValue(130, 280));
  input.push_back(TimestampValue(190, 460));
  input.push_back(TimestampValue(240, 460));
  input.push_back(TimestampValue(250, 710));
  input.push_back(TimestampValue(305, 840));
  input.push_back(TimestampValue(470, 1034));
  input.push_back(TimestampValue(900, 1630));

  // Expect a uniform set out
  vector<TimestampValue> expected;
  expected.push_back(TimestampValue(60,  109.31035));
  expected.push_back(TimestampValue(120, 255.0000));
  expected.push_back(TimestampValue(180, 430.0000));
  expected.push_back(TimestampValue(240, 460.0000));
  expected.push_back(TimestampValue(300, 828.1818));
  expected.push_back(TimestampValue(360, 904.6667));
  expected.push_back(TimestampValue(420, 975.2121));
  expected.push_back(TimestampValue(480, 1047.8605));
  expected.push_back(TimestampValue(540, 1131.0233));
  expected.push_back(TimestampValue(600, 1214.1860));
  expected.push_back(TimestampValue(660, 1297.3488));
  expected.push_back(TimestampValue(720, 1380.5116));
  expected.push_back(TimestampValue(780, 1463.6744));
  expected.push_back(TimestampValue(840, 1546.8372));
  expected.push_back(TimestampValue(900, 1630.0000));

  UniformTimeSeries ts(60);
  vector<TimestampValue> output;
  BOOST_FOREACH(TimestampValue &val, input) {
    proto::ValueStream stream = ts.AddPoint(val.timestamp, val.value);
    for (int i = 0; i < stream.value_size(); i++) {
      output.push_back(TimestampValue(stream.value(i).timestamp(), stream.value(i).double_value()));
    }
  }

  BOOST_FOREACH(TimestampValue &out, output) {
    ASSERT_GT(expected.size(), 0);
    TimestampValue exp = expected.front();
    expected.erase(expected.begin());
    VLOG(1) << "Expecting " << exp.timestamp << " / " << exp.value;
    EXPECT_EQ(exp.timestamp, out.timestamp);
    EXPECT_FLOAT_EQ(exp.value, out.value);
  }
  EXPECT_EQ(0, expected.size()) << "Not all expected values were returned";
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}



