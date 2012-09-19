/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <string>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include "lib/common.h"
#include "lib/file.h"
#include "lib/string.h"

namespace openinstrument {

class FileTest : public ::testing::Test {};

#define FILENAME "test_local_file.txt"

TEST_F(FileTest, CreateFile) {
  File fh(FILENAME, "w");
  EXPECT_EQ(FILENAME, fh.filename());
  EXPECT_EQ(18, fh.Write("This is some text\n"));
  EXPECT_EQ(23, fh.Write("This is some more text\n"));
  fh.Close();
}

TEST_F(FileTest, StatFile) {
  FileStat fs(FILENAME);
  EXPECT_EQ(41, fs.size());
}

TEST_F(FileTest, ReadFile) {
  File fh(FILENAME, "r");
  char buf[40] = {0};
  EXPECT_EQ(18, fh.Read(buf, 18));
  EXPECT_EQ(23, fh.Read(buf, 39));
  EXPECT_EQ(0, fh.Read(buf, 39));
  EXPECT_EQ(0, strcmp("This is some more text\n", buf));
  fh.Close();
}

TEST_F(FileTest, MmapReadFile) {
  MmapFile fh(FILENAME);
  char buf[40] = {0};
  fh.Read(0, 18, buf);
  EXPECT_EQ("This is some text\n", buf);
  fh.Close();
}

TEST_F(FileTest, UnlinkFile) {
  unlink(FILENAME);
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


