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
#include "lib/string.h"

namespace openinstrument {

class StringTest : public ::testing::Test {};

TEST_F(StringTest, PrintF) {
  string output(StringPrintf("%05d %7s %0.2f", 5, "test", 15.12345));
  EXPECT_EQ("00005    test 15.12", output);
}

TEST_F(StringTest, AppendF) {
  string output(StringPrintf("%05d %7s %0.2f", 5, "test", 15.12345));
  StringAppendf(&output, " %0.5f", 3.14159265);
  EXPECT_EQ("00005    test 15.12 3.14159", output);
}

TEST_F(StringTest, StringPieceTest) {
  string teststr1("This is a test string");
  string teststr2("Another String");
  string emptystring;
  StringPiece sp1(teststr1), sp2(sp1.data(), sp1.size()), sp3(teststr2), sp4(emptystring), sp5(sp1);
  EXPECT_TRUE(sp1 == sp2);
  EXPECT_EQ(21U, teststr1.size());
  EXPECT_EQ(0, strncmp("This is a test string", sp1.data(), teststr1.size()));
  EXPECT_EQ("This is a test string", sp1.ToString());
  EXPECT_EQ("This is a test string", sp5.ToString());
  EXPECT_FALSE(sp2 == sp3);
  EXPECT_TRUE(sp1 != sp3);
  EXPECT_TRUE(sp1);
  EXPECT_FALSE(sp4);
  EXPECT_EQ('h', sp1[1]);
  EXPECT_THROW(sp1[50], out_of_range);
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

