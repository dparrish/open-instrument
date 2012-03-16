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
#include "lib/common.h"
#include "lib/exported_vars.h"

namespace openinstrument {

class ExportedVarsTest : public ::testing::Test {};

TEST_F(ExportedVarsTest, ExportedInteger) {
  // Test the basic integer operations
  ExportedInteger exp("/openinstrument/test/expint", 0);
  EXPECT_EQ(0, exp);
  ++exp;
  EXPECT_EQ(1, exp);
  exp += 10;
  EXPECT_EQ(11, exp);
  exp = 15;
  EXPECT_EQ(15, exp);
  --exp;
  EXPECT_EQ(14, exp);

  // Test the initial value
  ExportedInteger exp2("/openinstrument/test/expint2", 1);
  EXPECT_EQ(1, exp2);

  // Export to a string
  string output;
  VariableExporter::GetGlobalExporter()->ExportToString(&output);
  EXPECT_NE(string::npos, output.find("/openinstrument/test/expint\t14\n"));
  EXPECT_NE(string::npos, output.find("/openinstrument/test/expint2\t1\n"));
}

TEST_F(ExportedVarsTest, GlobalExporter) {
  ExportedInteger exp("/openinstrument/test/expint", 5);
  ExportedInteger exp2("/openinstrument/test/expint2", 15);
  string output;
  VariableExporter::GetGlobalExporter()->ExportToString(&output);
  EXPECT_NE(string::npos, output.find("/openinstrument/test/expint\t5\n"));
  EXPECT_NE(string::npos, output.find("/openinstrument/test/expint2\t15\n"));
}

TEST_F(ExportedVarsTest, ExportedRatio) {
  ExportedRatio exp("/openinstrument/test/ratio");
  exp.success();
  exp.success();
  exp.failure();
  string output;
  VariableExporter::GetGlobalExporter()->ExportToString(&output);
  EXPECT_NE(string::npos, output.find("/openinstrument/test/ratio-total\t3\n"));
  EXPECT_NE(string::npos, output.find("/openinstrument/test/ratio-success\t2\n"));
  EXPECT_NE(string::npos, output.find("/openinstrument/test/ratio-failure\t1\n"));
}

TEST_F(ExportedVarsTest, ExportedAverage) {
  ExportedAverage exp("/openinstrument/test/average");
  exp.update(15, 1);
  exp.update(30, 2);
  string output;
  VariableExporter::GetGlobalExporter()->ExportToString(&output);
  EXPECT_NE(string::npos, output.find("/openinstrument/test/average-total-count\t3\n"));
  EXPECT_NE(string::npos, output.find("/openinstrument/test/average-overall-sum\t45\n"));
}

class FakeTimer : public Timer {
 public:
  explicit FakeTimer(int64_t elapsed) : Timer() {
    start_time_ = 1;
    elapsed_ = elapsed;
  }
  virtual ~FakeTimer() {}
};

TEST_F(ExportedVarsTest, ExportedTimer) {
  ExportedTimer exp("/openinstrument/test/timer");
  FakeTimer timer(1000);
  exp.update(timer);
  exp.update(timer);
  string output;
  VariableExporter::GetGlobalExporter()->ExportToString(&output);
  EXPECT_NE(string::npos, output.find("/openinstrument/test/timer-total-count\t2\n"));
  EXPECT_NE(string::npos, output.find("/openinstrument/test/timer-overall-sum{units=ms}\t2000\n"));
}

TEST_F(ExportedVarsTest, ExportedIntegerSet) {
  ExportedIntegerSet exp("/openinstrument/test");
  ++exp["/item1"];
  exp["/item2"] += 10;
  string output;
  VariableExporter::GetGlobalExporter()->ExportToString(&output);
  EXPECT_NE(string::npos, output.find("/openinstrument/test/item1\t1\n"));
  EXPECT_NE(string::npos, output.find("/openinstrument/test/item2\t10\n"));
}


}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

