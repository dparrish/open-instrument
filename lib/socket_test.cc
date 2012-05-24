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
  EXPECT_THROW(addr.port(), std::runtime_error);
  EXPECT_THROW(addr.AddressToString(), std::runtime_error);
  EXPECT_THROW(addr.ToString(), std::runtime_error);
}

TEST_F(SocketTest, StringAddress1) {
  Socket::Address addr("192.168.1.1", 8010);
  EXPECT_EQ(8010, addr.port());
  EXPECT_EQ("192.168.1.1", addr.AddressToString());
  EXPECT_EQ("192.168.1.1:8010", addr.ToString());
}

TEST_F(SocketTest, StringAddress2) {
  Socket::Address addr("192.168.1.1:8010");
  EXPECT_EQ(8010, addr.port());
  EXPECT_EQ("192.168.1.1", addr.AddressToString());
  EXPECT_EQ("192.168.1.1:8010", addr.ToString());
}

TEST_F(SocketTest, IntAddress) {
  Socket::Address addr(3232235777U, 8010);
  EXPECT_EQ(8010, addr.port());
  EXPECT_EQ("192.168.1.1", addr.AddressToString());
  EXPECT_EQ("192.168.1.1:8010", addr.ToString());
}

TEST_F(SocketTest, AddressCopy) {
  Socket::Address addr("192.168.1.1", 8010);
  Socket::Address addr2(addr);
  EXPECT_EQ("192.168.1.1:8010", addr2.ToString());
}

TEST_F(SocketTest, V6Address) {
  Socket::Address addr("2001:44b8::793e", 8010);
  EXPECT_EQ("[2001:44b8::793e]:8010", addr.ToString());
  EXPECT_EQ("2001:44b8::793e", addr.AddressToString());
}

TEST_F(SocketTest, V6Address2) {
  Socket::Address addr("[2001:44b8::793e]", 8010);
  EXPECT_EQ("[2001:44b8::793e]:8010", addr.ToString());
  EXPECT_EQ("2001:44b8::793e", addr.AddressToString());
}


TEST_F(SocketTest, V6AddressPort) {
  Socket::Address addr("[2001:44b8::793e]:8010");
  EXPECT_EQ("[2001:44b8::793e]:8010", addr.ToString());
  EXPECT_EQ("2001:44b8::793e", addr.AddressToString());
}

TEST_F(SocketTest, Resolve) {
  Socket sock;
  vector<Socket::Address> addrs = sock.Resolve("localhost");
  int found = 0;
  for (auto &addr : addrs) {
    if (addr.AddressToString() == "127.0.0.1" || addr.AddressToString() == "::1")
      found++;
  }
  EXPECT_GE(found, 2);
}

TEST_F(SocketTest, ReverseResolve) {
  vector<string> addrs = Socket::ReverseResolve(Socket::Address("127.0.0.1"));
  bool found = false;
  for (string &addr : addrs) {
    if (addr.find("localhost") != string::npos)
      found = true;
  }
  EXPECT_TRUE(found);
}

TEST_F(SocketTest, LocalAddrs) {
  vector<Socket::Address> addrs = Socket::LocalAddresses();
  int found = 0;
  for (auto &addr : addrs) {
    if (addr.AddressToString() == "127.0.0.1" || addr.AddressToString() == "::1")
      found++;
  }
  EXPECT_GE(found, 2);
}

TEST_F(SocketTest, Listen) {
  Socket sock;
  sock.Listen(Socket::Address("::", 8022));
  LOG(INFO) << "Listening on " << sock.local().ToString();
  scoped_ptr<Socket> client(sock.Accept(0));
  EXPECT_FALSE(client.get());
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


