/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <iostream>
#include <string>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include "lib/common.h"
#include "lib/file.h"
#include "lib/openinstrument.pb.h"
#include "lib/protobuf.h"
#include "lib/string.h"
#include "lib/timer.h"
#include "lib/variable.h"

namespace openinstrument {

class VariableTest : public ::testing::Test {};

TEST_F(VariableTest, Variable) {
  {
    Variable foo("/test/variable/1");
    EXPECT_EQ("/test/variable/1", foo.ToString());
  }
  {
    Variable foo("/test/variable/2{label2=\"valu\\\"e 2\",label1=value1}");
    EXPECT_EQ("/test/variable/2", foo.variable());
    EXPECT_EQ("value1", foo.GetLabel("label1"));
    EXPECT_EQ("valu\"e 2", foo.GetLabel("label2"));
    EXPECT_EQ("", foo.GetLabel("label3"));
    EXPECT_EQ(true, foo.HasLabel("label1"));
    EXPECT_TRUE(false == foo.HasLabel("label3"));
    EXPECT_EQ("/test/variable/2{label1=value1,label2=\"valu\\\"e 2\"}", foo.ToString());
  }
}

TEST_F(VariableTest, VariableMatches) {
  Variable input("/test/variable/1{label1=foobar,label2=barfoo,label3=1219827391}");
  EXPECT_TRUE(input.Matches("/test/variable/1"));
  EXPECT_FALSE(input.Matches("/test/variable/2"));
  EXPECT_TRUE(input.Matches("/test/varia*"));
  EXPECT_FALSE(input.Matches("/test/notvaria*"));
  EXPECT_TRUE(input.Matches("/test/variable/1{}"));
  EXPECT_TRUE(input.Matches("/test/variable/1{label1=*}"));
  EXPECT_FALSE(input.Matches("/test/variable/1{label4=*}"));
  EXPECT_TRUE(input.Matches("/test/variable/1{label1=foobar}"));
  EXPECT_FALSE(input.Matches("/test/variable/1{label1=barfoo}"));
  EXPECT_TRUE(input.Matches("/test/variable/1{label1=/foo.*/}"));
  EXPECT_FALSE(input.Matches("/test/variable/1{label1=/foo/}"));
  EXPECT_TRUE(input.Matches("/test/variable/1{label1=/foo.*/}"));
  EXPECT_FALSE(input.Matches("/test/variable/1{label4=yay}"));
  EXPECT_TRUE(input == "/test/variable/1{label1=foobar,label2=barfoo,label3=1219827391}");
  EXPECT_TRUE(input.equals("/test/variable/1{label2=barfoo,label3=1219827391,label1=foobar}"));
  EXPECT_FALSE(input.equals("/test/variable/1{label3=1219827391,label1=foobar}"));
  EXPECT_FALSE(input.equals("/test/variable/1{label1=foobar,label2=barfoo,label3=1298371927}"));
  EXPECT_FALSE(input.equals("/test/variable/1{label1=foobar,label2=barfoo,label3=1219827391,label4=yay}"));
}

TEST_F(VariableTest, Protobuf) {
  proto::StreamVariable protobuf;

  Variable input("/test/variable/1{label1=foobar,label3=barfoo,label2=1219827391}");
  input.ToProtobuf(&protobuf);

  Variable output(protobuf);
  EXPECT_TRUE(input.equals(output));
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


