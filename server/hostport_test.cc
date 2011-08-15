/*
 *  -
 *
 * (c) 2010 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <string>
#include <gtest/gtest.h>
#include "hostport.h"

namespace openinstrument {

class HostPortTest : public ::testing::Test {};

TEST_F(HostPortTest, CacheTest) {
  HostPort *source1 = HostPort::get("foobar", 8000);
  ASSERT_TRUE(source1);
  HostPort *source2 = HostPort::get("foobar", 8001);
  ASSERT_TRUE(source2);
  HostPort *source3 = HostPort::get("foobar", 8000);
  ASSERT_TRUE(source3);
  ASSERT_EQ(source1, source3);
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

