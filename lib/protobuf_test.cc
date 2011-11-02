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

namespace openinstrument {

class ProtobufTest : public ::testing::Test {};

void CorruptFile(const string &filename, int num_bytes = 1) {
  File fh(filename, "r+");
  for (int i = 0; i < num_bytes; i++) {
    fh.SeekAbs(rand() % fh.stat().size());
    char randbyte = rand();
    fh.Write(&randbyte, 1);
  }
}

TEST_F(ProtobufTest, WriteCorruptRead) {
  string filename("protobuf_test_output.txt");
  unlink(filename.c_str());

  ProtoStreamWriter writer(filename);
  for (int i = 0; i < 200; i++) {
    proto::GetRequest req;
    Variable var(StringPrintf("/openinstrument/variable%d", i));
    var.CopyTo(req.mutable_variable());
    req.set_min_timestamp(12345);
    req.set_max_timestamp(67890);
    writer.Write(req);
  }

  srand(time(NULL));
  int num_res = 0;
  for (int i = 1; i < 100; i++) {
    num_res = 0;
    ProtoStreamReader reader(filename);
    proto::GetRequest req;
    while (reader.Next(&req)) {
      num_res++;
    }
    ASSERT_GT(num_res, 0);
    CorruptFile(filename, 10);
  }
  EXPECT_LT(num_res, 200);
  EXPECT_GE(num_res, 1);
}

TEST_F(ProtobufTest, Variable) {
  {
    Variable foo("/test/variable/1");
    EXPECT_EQ("/test/variable/1", foo.ToString());
  }
  {
    Variable foo("/test/variable/2{label2=\"valu\\\"e 2\",label1=value1}");
    EXPECT_EQ("/test/variable/2", foo.name());
    EXPECT_EQ("value1", foo.GetLabel("label1"));
    EXPECT_EQ("valu\"e 2", foo.GetLabel("label2"));
    EXPECT_EQ("", foo.GetLabel("label3"));
    EXPECT_EQ(true, foo.HasLabel("label1"));
    EXPECT_TRUE(false == foo.HasLabel("label3"));
    EXPECT_EQ("/test/variable/2{label1=value1,label2=\"valu\\\"e 2\"}", foo.ToString());
  }
}

TEST_F(ProtobufTest, VariableMatches) {
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

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


