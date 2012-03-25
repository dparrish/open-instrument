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
#include <netinet/in.h>
#include "lib/common.h"
#include "lib/cord.h"
#include "lib/string.h"

namespace openinstrument {

class Socket : private noncopyable {
 public:
  class Address {
   public:
    Address();
    Address(uint32_t address, uint16_t port);
    Address(const string &address, uint16_t port);
    explicit Address(const string &serverport);
    void FromString(const string &addr);
    string ToString() const;
    string AddressToString() const;
    void Clear();
    bool valid() const;

    inline bool v4() const {
      return address_.ss_family == AF_INET;
    }

    inline bool v6() const {
      return address_.ss_family == AF_INET6;
    }

    inline const sockaddr_in *const_sockaddr_v4() const {
      return reinterpret_cast<const sockaddr_in *>(&address_);
    }

    inline const sockaddr_in6 *const_sockaddr_v6() const {
      return reinterpret_cast<const sockaddr_in6 *>(&address_);
    }

    inline sockaddr_in *sockaddr_v4() {
      return reinterpret_cast<sockaddr_in *>(&address_);
    }

    inline sockaddr_in6 *sockaddr_v6() {
      return reinterpret_cast<sockaddr_in6 *>(&address_);
    }

    inline void set_port(uint16_t port) {
      if (v4())
        sockaddr_v4()->sin_port = htons(port);
      else if (v6())
        sockaddr_v6()->sin6_port = htons(port);
      else
        throw runtime_error("No address type defined for socket");
    }

    inline uint16_t port() const {
      if (v4())
        return ntohs(const_sockaddr_v4()->sin_port);
      else if (v6())
        return ntohs(const_sockaddr_v6()->sin6_port);
      else
        throw runtime_error("No address type defined for socket");
    }

    struct sockaddr_storage address_;
  };

  Socket() : fd_(0) {}
  explicit Socket(int fd) : fd_(fd) {}

  ~Socket() {
    Close();
  }

  static vector<Address> Resolve(const char *hostname);
  static vector<string> ReverseResolve(const Address &address);
  static vector<Address> LocalAddresses();

  void Listen(const Address &addr);
  Socket *Accept(int timeout = -1) const;
  void SetNonblocking(bool nonblocking = true);
  void AttemptFlush(int timeout = 0);
  uint64_t Read(int timeout = -1);
  void Abort();
  void Connect(Address addr, int timeout = -1);
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

  inline void Write(const StringPiece &str) {
    write_buffer_.CopyFrom(str.data(), str.size());
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

  // Return the current hostname
  static string Hostname();

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
