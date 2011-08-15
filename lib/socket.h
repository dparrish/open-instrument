/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_SOCKET_H_
#define _OPENINSTRUMENT_LIB_SOCKET_H_

#include <string>
#include <vector>
#include "lib/common.h"
#include "lib/cord.h"
#include "lib/string.h"

namespace openinstrument {

class Socket {
 public:
  class Address {
   public:
    Address() : address(0), port(0) {}
    Address(uint32_t address, uint16_t port) : address(address), port(port) {}
    Address(const string &address, uint16_t port) : address(AddressFromString(address)), port(port) {}
    Address(const Address &src) : address(src.address), port(src.port) {}
    explicit Address(const string &serverport);

    inline string ToString() const {
      return StringPrintf("%s:%u", AddressToString(address).c_str(), port);
    }

    inline string AddressToString() const {
      return AddressToString(address);
    }

    uint32_t AddressFromString(const string &addr);
    static string AddressToString(uint32_t addr);

    // WARNING: Address and port are stored in host byte order and must be converted to network byte order before use by
    // most starndard library functions.
    uint32_t address;
    uint16_t port;
  };

  Socket() : fd_(0) {}
  explicit Socket(int fd) : fd_(fd) {}

  ~Socket() {
    Close();
  }

  void Listen(const Address &addr);
  Socket *Accept(int timeout = -1) const;
  void SetNonblocking(bool nonblocking = true);
  static vector<Address> Resolve(const char *hostname);
  void AttemptFlush(int timeout = 0);
  uint64_t Read(int timeout = -1);
  void Abort();
  void Connect(const Address &addr, int timeout = -1);
  void Connect(const string &address, uint16_t port, int timeout = -1);

  void Flush() {
    // Wait forever to flush output
    AttemptFlush(-1);
  }

  inline void Close() {
    Flush();
    Abort();
  }

  inline void Write(const string &str) {
    write_buffer_.CopyFrom(str);
    AttemptFlush();
    Read(0);
  }

  inline void Write(const Cord &cord) {
    write_buffer_.CopyFrom(cord);
    AttemptFlush();
    Read(0);
  }

  inline bool PollRead(int timeout) const;
  inline bool PollWrite(int timeout) const;

  inline const Address &local() const {
    return local_;
  }

  inline const Address &remote() const {
    return remote_;
  }

  Cord *read_buffer() {
    return &read_buffer_;
  }

 private:
  int fd_;
  Address local_;
  Address remote_;
  Cord write_buffer_;
  Cord read_buffer_;

  bool Poll(int timeout, uint16_t events) const;

  Address *mutable_remote() {
    return &remote_;
  }

  Address *mutable_local() {
    return &local_;
  }
};

}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_SOCKET_H_
