/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <iostream>
#include <fstream>
#include <string>
#include <gtest/gtest.h>
#include "lib/hash.h"
#include "lib/string.h"

namespace openinstrument {

class HashTest : public ::testing::Test {};

void gen_random(char *s, const int len) {
  static const char alphanum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";

  for (int i = 0; i < len; ++i) {
    s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
  }

  s[len] = 0;
}

TEST_F(HashTest, HashTest) {
  EXPECT_EQ(2119216193UL, Hash::Hash32("This is a test string"));
  EXPECT_EQ(9050795318103288897ULL, Hash::Hash64("This is a test string"));
  uint32_t pc = 0, pb = 0;
  Hash::DoubleHash32("This is a test string", &pc, &pb);
  EXPECT_EQ(2119216193UL, pc);
  EXPECT_EQ(2107302499UL, pb);
}

TEST_F(HashTest, ConsistentHashTest) {
  HashRing<string> ring(2);
  ring.AddNode("test1.example.com");
  ring.AddNode("test2.example.com");
  ring.AddNode("test3.example.com");
  ring.AddNode("test4.example.com");
  int node1 = 0, node2 = 0, node3 = 0, node4 = 0;

  char str[50];
  for (int i = 0; i < 1000; i++) {
    gen_random(str, 49);
    string key = StringPrintf("sample key %d: %s", i, str);
    string node = ring.GetNode(key);
    if (node == "test1.example.com")
      node1++;
    else if (node == "test2.example.com")
      node2++;
    else if (node == "test3.example.com")
      node3++;
    else if (node == "test4.example.com")
      node4++;
    CHECK_NE(ring.GetNode(key), ring.GetBackupNode(key));
  }
  LOG(INFO) << "test1.example.com: " << node1;
  LOG(INFO) << "test2.example.com: " << node2;
  LOG(INFO) << "test3.example.com: " << node3;
  LOG(INFO) << "test4.example.com: " << node4;
  EXPECT_GT(node1, 0);
  EXPECT_GT(node2, 0);
  EXPECT_GT(node3, 0);
  EXPECT_GT(node4, 0);
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


