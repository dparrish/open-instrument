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
  for (int i = 0; i < 1000; i++) {
    proto::GetRequest req;
    req.set_variable(StringPrintf("/openinstrument/variable%d", i));
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
  EXPECT_LT(num_res, 1000);
  EXPECT_GE(num_res, 1);
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


