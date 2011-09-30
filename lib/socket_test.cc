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
#include "lib/cord.h"
#include "lib/socket.h"
#include "lib/string.h"

namespace openinstrument {

class SocketTest : public ::testing::Test {};

TEST_F(SocketTest, EmptyAddress) {
  Socket::Address addr;
  EXPECT_EQ(0, addr.address);
  EXPECT_EQ(0, addr.port);
  EXPECT_EQ("0.0.0.0", addr.AddressToString());
  EXPECT_EQ("0.0.0.0:0", addr.ToString());
}

TEST_F(SocketTest, StringAddress1) {
  Socket::Address addr("192.168.1.1", 8010);
  EXPECT_EQ(3232235777U, addr.address);
  EXPECT_EQ(8010, addr.port);
  EXPECT_EQ("192.168.1.1", addr.AddressToString());
  EXPECT_EQ("192.168.1.1:8010", addr.ToString());
}

TEST_F(SocketTest, StringAddress2) {
  Socket::Address addr("192.168.1.1:8010");
  EXPECT_EQ(3232235777U, addr.address);
  EXPECT_EQ(8010, addr.port);
  EXPECT_EQ("192.168.1.1", addr.AddressToString());
  EXPECT_EQ("192.168.1.1:8010", addr.ToString());
}

TEST_F(SocketTest, IntAddress) {
  Socket::Address addr(3232235777U, 8010);
  EXPECT_EQ(3232235777U, addr.address);
  EXPECT_EQ(8010, addr.port);
  EXPECT_EQ("192.168.1.1", addr.AddressToString());
  EXPECT_EQ("192.168.1.1:8010", addr.ToString());
}

TEST_F(SocketTest, AddressCopy) {
  Socket::Address addr("192.168.1.1", 8010);
  Socket::Address addr2(addr);
  EXPECT_EQ("192.168.1.1:8010", addr2.ToString());
}

TEST_F(SocketTest, Resolve) {
  Socket sock;
  vector<Socket::Address> addrs = sock.Resolve("localhost");
  bool found = false;
  BOOST_FOREACH(Socket::Address &addr, addrs) {
    if (addr.AddressToString() == "127.0.0.1") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(SocketTest, Listen) {
  Socket sock;
  sock.Listen(Socket::Address("0.0.0.0", 8022));
  LOG(INFO) << "Listening on " << sock.local().ToString();

  scoped_ptr<Socket> client(sock.Accept(1));
  EXPECT_FALSE(client.get());
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


