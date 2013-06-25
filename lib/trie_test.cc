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
#include "lib/trie.h"

namespace openinstrument {

class TrieTest : public ::testing::Test {};

TEST_F(TrieTest, Insert) {
  Trie<string> trie;
  EXPECT_EQ(0UL, trie.size());
  EXPECT_TRUE(trie.empty());
  trie.insert("/openinstrument/test/key", "test value");
  trie.insert("/openinstrument/test/key2", "test value 2");
  trie.insert("/openinstrument/test/key3", "test value 3");
  trie.insert("/xopeninstrument/other", "test value 2");
  trie.insert("/apeninstrument/foo", "bar");
  // This last entry replaces the previous one so should not increase the size
  trie.insert("/apeninstrument/foo", "foo");
  EXPECT_EQ(5UL, trie.size());
  EXPECT_FALSE(trie.empty());
}

TEST_F(TrieTest, Iterator) {
  Trie<string> trie;
  for (int i = 0; i < 10; i++) {
    trie.insert(StringPrintf("/openinstrument/test/key%d", i), StringPrintf("test value %d", i));
  }
  EXPECT_EQ(10UL, trie.size());
  int count = 0;
  string laststring = "";
  // Iterate over all the items and verify that they are ordered correctly
  for (auto i : trie) {
    EXPECT_TRUE(i > laststring);
    laststring = i;
    count++;
  }
  EXPECT_EQ(10, count);
  count = 0;
  for (string str : trie)
    count++;
  EXPECT_EQ(10, count);
}

TEST_F(TrieTest, Clear) {
  Trie<string> trie;
  for (int i = 0; i < 10; i++) {
    trie.insert(StringPrintf("/openinstrument/test/key%d", i), StringPrintf("test value %d", i));
  }
  EXPECT_EQ(10UL, trie.size());
  trie.clear();
  EXPECT_EQ(0UL, trie.size());
  for (auto i : trie) {
    FAIL() << "There should not be any entries: " << i;
  }
}

TEST_F(TrieTest, Find) {
  Trie<string> trie;
  for (int i = 0; i < 10; i++) {
    trie.insert(StringPrintf("/openinstrument/test/key%d", i), StringPrintf("test value %d", i));
  }
  Trie<string>::iterator it = trie.find("/openinstrument/test/key5");
  EXPECT_EQ(*it, "test value 5");
  it = trie.find("/openinstrument/test/key15");
  EXPECT_TRUE(it == trie.end());
}

TEST_F(TrieTest, FindPrefix) {
  Trie<string> trie;
  for (int i = 0; i < 10; i++) {
    trie.insert(StringPrintf("/openinstrument/test/key%d", i), StringPrintf("test value %d", i));
    trie.insert(StringPrintf("/openinstrument/testx/key%d", i), StringPrintf("testx value %d", i));
  }
  int count = 0;
  Trie<string>::iterator i = trie.find_prefix("/openinstrument/testx");
  EXPECT_EQ("testx value 0", *i);
  for (; i != trie.end(); ++i)
    count++;
  EXPECT_EQ(10, count);
}

TEST_F(TrieTest, Erase) {
  Trie<string> trie;
  for (int i = 0; i < 10; i++) {
    trie.insert(StringPrintf("/openinstrument/test/key%d", i), StringPrintf("test value %d", i));
  }
  // This shouldn't delete anything
  trie.erase("test value 1");
  EXPECT_EQ(10UL, trie.size());
  trie.erase("/openinstrument/test/key1");
  EXPECT_EQ(9UL, trie.size());
  Trie<string>::iterator i = trie.find("/openinstrument/test/key1");
  EXPECT_TRUE(trie.end() == i);
}

TEST_F(TrieTest, SquareBrackets) {
  Trie<string> trie;
  for (int i = 0; i < 10; i++) {
    trie.insert(StringPrintf("/openinstrument/test/key%d", i), StringPrintf("test value %d", i));
  }
  EXPECT_EQ("test value 1", trie["/openinstrument/test/key1"]);
  EXPECT_THROW(trie["test value 1"], out_of_range);
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}



