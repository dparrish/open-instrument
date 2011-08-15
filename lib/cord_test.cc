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
#include "lib/cord.h"
#include "lib/string.h"

namespace openinstrument {

class CordTest : public ::testing::Test {};

TEST_F(CordTest, GetAppendBuf) {
  Cord cord;
  ASSERT_EQ(0, cord.size());
  char *buf = NULL;
  uint32_t size = 0;
  string appendstring("This is the string that will be appended.\n");
  for (int i = 0; i < 10; i++) {
    cord.GetAppendBuf(appendstring.size(), &buf, &size);
    EXPECT_EQ(appendstring.size(), size);
    memcpy(buf, appendstring.c_str(), appendstring.size());
  }
  EXPECT_EQ(420, cord.size());
}

TEST_F(CordTest, Append) {
  Cord cord;
  string teststring("jfdkjasxxxxhilfuhawelsuehfliwnevlkasuhvjnvkjlawekrnlkasuhiawhvkjahlkjskljhflwkevcnlkjsZNLDvsdx\n");
  for (int i = 0; i < 20; i++) {
    cord.Append(teststring);
  }
  EXPECT_EQ(1900, cord.size());

  string output;
  cord.AppendTo(&output);
  EXPECT_EQ(1900, output.size());
}

TEST_F(CordTest, CopyCord) {
  Cord cord;
  string appendstring("This is the string that will be appended.\n");
  char *buf = NULL;
  uint32_t size = 0;
  for (int i = 0; i < 10; i++) {
    cord.GetAppendBuf(appendstring.size(), &buf, &size);
    EXPECT_EQ(appendstring.size(), size);
    memcpy(buf, appendstring.c_str(), appendstring.size());
  }

  Cord newcord(cord);
  EXPECT_EQ(newcord.size(), cord.size());

  string output, newoutput;
  cord.AppendTo(&output);
  newcord.AppendTo(&newoutput);

  EXPECT_EQ(newoutput, output);
}

TEST_F(CordTest, SubstrPointers) {
  Cord cord;
  cord.set_default_buffer_size(16);
  string teststring("1234567890");
  for (int i = 0; i < 20; i++) {
    cord.CopyFrom(teststring.c_str(), teststring.size());
  }
  EXPECT_EQ(200, cord.size());

  // Start after bounds
  EXPECT_THROW(cord.Substr(400, 10), out_of_range);
  // A whole single block
  EXPECT_EQ("1234567890", cord.Substr(0, 10));
  // Part of a block and the next
  EXPECT_EQ("2345678901", cord.Substr(1, 10));
  // Part of a block and the next, starting from a way into the Cord
  EXPECT_EQ("2345678901", cord.Substr(21, 10));
  // Past the end of the Cord
  EXPECT_EQ("67890", cord.Substr(195, 10));
  // Multiple blocks, starting inside a block
  EXPECT_EQ("456789012345678901234567890123", cord.Substr(33, 30));

  string output;
  cord.Substr(0, 10, &output);
  cord.Substr(1, 10, &output);
  EXPECT_EQ("12345678902345678901", output);
}

TEST_F(CordTest, SubstrStrings) {
  Cord cord;
  cord.set_default_buffer_size(16);
  string teststring("1234567890");
  for (int i = 0; i < 20; i++) {
    cord.CopyFrom(teststring.c_str(), teststring.size());
  }
  EXPECT_EQ(200, cord.size());

  // Start after bounds
  EXPECT_THROW(cord.Substr(400, 10), out_of_range);
  // A whole single block
  EXPECT_EQ("1234567890", cord.Substr(0, 10));
  // Part of a block and the next
  EXPECT_EQ("2345678901", cord.Substr(1, 10));
  // Part of a block and the next, starting from a way into the Cord
  EXPECT_EQ("2345678901", cord.Substr(21, 10));
  // Past the end of the Cord
  EXPECT_EQ("67890", cord.Substr(195, 10));
  // Multiple blocks, starting inside a block
  EXPECT_EQ("456789012345678901234567890123", cord.Substr(33, 30));

  string output;
  cord.Substr(0, 10, &output);
  cord.Substr(1, 10, &output);
  EXPECT_EQ("12345678902345678901", output);
}

TEST_F(CordTest, RandomAccess) {
  Cord cord;
  cord.set_default_buffer_size(16);
  string teststring("1234567890");
  for (int i = 0; i < 20; i++) {
    cord.Append(teststring);
  }
  EXPECT_EQ(200, cord.size());

  EXPECT_EQ('1', cord[0]);
  EXPECT_EQ('0', cord[9]);
  EXPECT_EQ('2', cord[11]);
  EXPECT_EQ('0', cord[199]);
  EXPECT_THROW(cord[200], out_of_range);
}

TEST_F(CordTest, IteratorRead) {
  Cord cord;
  cord.set_default_buffer_size(16);
  cord.set_minimum_append_size(1);
  string teststring("1234567890");
  for (int i = 0; i < 20; i++) {
    cord.CopyFrom(teststring.c_str(), teststring.size());
  }
  EXPECT_EQ(200, cord.size());

  Cord::iterator i = cord.begin();
  EXPECT_EQ("1234567890123456", string(i->buffer(), i->size()));
  ++i;
  EXPECT_EQ("7890123456789012", string(i->buffer(), i->size()));
  ++i;
  EXPECT_EQ("3456789012345678", string(i->buffer(), i->size()));
  ++i;
  EXPECT_EQ("9012345678901234", string(i->buffer(), i->size()));
  ++i;
  EXPECT_EQ("5678901234567890", string(i->buffer(), i->size()));
  ++i;
  EXPECT_EQ("1234567890123456", string(i->buffer(), i->size()));
  ++i;
  EXPECT_EQ("7890123456789012", string(i->buffer(), i->size()));
  ++i;
  EXPECT_EQ("3456789012345678", string(i->buffer(), i->size()));
  ++i;
  EXPECT_EQ("9012345678901234", string(i->buffer(), i->size()));
  ++i;
  EXPECT_EQ("5678901234567890", string(i->buffer(), i->size()));
  ++i;
  EXPECT_EQ("1234567890123456", string(i->buffer(), i->size()));
  ++i;
  EXPECT_EQ("7890123456789012", string(i->buffer(), i->size()));
  ++i;
  EXPECT_EQ("34567890", string(i->buffer(), i->size())); ++i;
}

TEST_F(CordTest, Equals) {
  Cord cord;
  cord.set_default_buffer_size(16);
  cord.set_minimum_append_size(1);
  string teststring("1234567890");
  for (int i = 0; i < 20; i++) {
    cord.CopyFrom(teststring.c_str(), teststring.size());
  }
  Cord newcord(cord);
  EXPECT_TRUE(cord == newcord);
}

TEST_F(CordTest, Consume) {
  Cord cord;
  cord.set_default_buffer_size(16);
  cord.set_minimum_append_size(1);
  cord.CopyFrom("Line 1\r\n");
  cord.CopyFrom("\r\n");  // empty line
  cord.CopyFrom("This is line 2, it is a long line that spans multiple CordBuffer blocks.\r\n");
  cord.CopyFrom("This is line 3 that does not have a trailing newline");

  string out;
  EXPECT_EQ(136, cord.size());
  EXPECT_EQ('L', cord[0]);
  cord.Consume(1, &out);
  EXPECT_EQ("L", out);
  EXPECT_EQ('i', cord[0]);
  EXPECT_EQ(135, cord.size());
  out.clear();
  cord.Consume(53, &out);
  EXPECT_EQ("ine 1\r\n\r\nThis is line 2, it is a long line that spans", out);
}

TEST_F(CordTest, ConsumeLine) {
  Cord cord;
  cord.set_default_buffer_size(16);
  cord.set_minimum_append_size(1);
  cord.CopyFrom("Line 1\r\n");
  cord.CopyFrom("Line 2\r\n");
  cord.CopyFrom("\r\n");  // empty line
  cord.CopyFrom("This is line 3, it is a long line that spans multiple CordBuffer blocks.\r\n");
  cord.CopyFrom("This is line 4 that does not have a trailing newline");

  EXPECT_EQ(144, cord.size());
  string output;
  cord.ConsumeLine(&output);
  EXPECT_EQ("Line 1", output);
  cord.ConsumeLine(&output);
  EXPECT_EQ("Line 1Line 2", output);
  output.clear();
  cord.ConsumeLine(&output);
  EXPECT_EQ("", output);
  output.clear();
  cord.ConsumeLine(&output);
  EXPECT_EQ("This is line 3, it is a long line that spans multiple CordBuffer blocks.", output);
  output.clear();
  EXPECT_THROW(cord.ConsumeLine(&output), out_of_range);
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


