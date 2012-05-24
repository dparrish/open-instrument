/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <algorithm>
#include <string>
#include <vector>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "lib/common.h"
#include "lib/socket.h"
#include "lib/timer.h"

namespace openinstrument {

vector<Socket::Address> Socket::Resolve(const char *hostname) {
  struct addrinfo *result;
  int s = getaddrinfo(hostname, NULL, NULL, &result);
  if (s != 0)
    throw runtime_error(StringPrintf("getaddrinfo %s: %s", hostname, gai_strerror(s)));
  vector<Socket::Address> results;
  for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
    if (rp->ai_family == AF_INET) {
      results.push_back(Address(ntohl(reinterpret_cast<struct sockaddr_in *>(rp->ai_addr)->sin_addr.s_addr), 0));
      results.back().address_.ss_family = rp->ai_family;
    } else if (rp->ai_family == AF_INET6) {
      Address addr;
      addr.address_.ss_family = rp->ai_family;
      memcpy(&addr.sockaddr_v6()->sin6_addr, &reinterpret_cast<struct sockaddr_in6 *>(rp->ai_addr)->sin6_addr,
             sizeof(addr.sockaddr_v6()->sin6_addr));
      results.push_back(addr);
    }
  }
  freeaddrinfo(result);

  return results;
}

vector<string> Socket::ReverseResolve(const Address &addr) {
  vector<string> ret;
  if (addr.v4()) {
    char host[NI_MAXHOST];
    if (getnameinfo(reinterpret_cast<const sockaddr *>(addr.const_sockaddr_v4()), sizeof(struct sockaddr_storage), host,
                    NI_MAXHOST, NULL, 0, NI_NAMEREQD) != 0) {
      LOG(WARNING) << "getnameinfo() failed: " << strerror(errno);
    } else {
      ret.push_back(host);
    }
  }
  return ret;
}

vector<Socket::Address> Socket::LocalAddresses() {
  struct ifaddrs *ifaddr;
  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    exit(EXIT_FAILURE);
  }

  vector<Socket::Address> ret;
  for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL)
      continue;

    if (ifa->ifa_addr->sa_family == AF_INET) {
      ret.push_back(Address(htonl(reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr)->sin_addr.s_addr), 0));
    } else if (ifa->ifa_addr->sa_family == AF_INET6) {
      Address addr;
      addr.address_.ss_family = ifa->ifa_addr->sa_family;
      memcpy(&addr.sockaddr_v6()->sin6_addr, &reinterpret_cast<struct sockaddr_in6 *>(ifa->ifa_addr)->sin6_addr,
             sizeof(addr.sockaddr_v6()->sin6_addr));
      ret.push_back(addr);
    } else {
      continue;
    }
  }

  freeifaddrs(ifaddr);
  return ret;
}

void Socket::Listen(const Socket::Address &addr) {
  local_ = addr;
  if ((fd_ = ::socket(local_.address_.ss_family, SOCK_STREAM, 0)) <= 0)
    throw runtime_error(StringPrintf("Can't create socket: %s", strerror(errno)));
  int on = 1;
  setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  if (::bind(fd_, (sockaddr *)local_.sockaddr_v4(), sizeof(local_.address_)) < 0)
    throw runtime_error(StringPrintf("Can't bind socket: %s", strerror(errno)));

  if (::listen(fd_, 100) < 0)
    throw runtime_error(StringPrintf("Can't listen on socket: %s", strerror(errno)));

  VLOG(1) << "Listening on " << local_.ToString();
}

void Socket::SetNonblocking(bool nonblocking) {
  int flags = fcntl(fd_, F_GETFL, 0);
  if (nonblocking)
    fcntl(fd_, F_SETFL, flags | O_NONBLOCK);
  else
    fcntl(fd_, F_SETFL, (flags | O_NONBLOCK) ^ O_NONBLOCK);
}

void Socket::Abort() {
  if (fd_)
    ::close(fd_);
  fd_ = 0;
  read_buffer_.clear();
  write_buffer_.clear();
}

Socket *Socket::Accept(int timeout) const {
  while (true) {
    // Wait up to <timeout> ms for a connection
    if (!PollRead(timeout)) {
      if (timeout >= 0)
        return NULL;
      continue;
    }

    Socket::Address addr;
    socklen_t len = sizeof(addr.address_);
    int connfd = accept(fd_, reinterpret_cast<struct sockaddr *>(&addr.address_), &len);
    if (connfd <= 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        continue;
      throw runtime_error(StringPrintf("Error accepting client connection: %s", strerror(errno)));
    }

    Socket *client = new Socket(connfd);
    memcpy(&client->mutable_local()->address_, &local_.address_, sizeof(local_.address_));
    memcpy(&client->mutable_remote()->address_, &addr.address_, sizeof(addr.address_));
    client->SetNonblocking(true);
    return client;
  }
}

void Socket::AttemptFlush(int timeout) {
  if (!fd_) {
    // Not connected
    return;
  }
  while (write_buffer_.size()) {
    if (!PollWrite(timeout)) {
      // Not ready to write
      return;
    }
    CordBuffer &buf = write_buffer_.front();
    if (!buf.size()) {
      write_buffer_.pop_front();
      continue;
    }
    for (const char *p = buf.buffer(); p < buf.buffer() + buf.size();) {
      int flags = 0;
      int64_t bytesleft = (buf.buffer() + buf.size() - p);
      if (bytesleft < 0) {
        LOG(ERROR) << "This is probably bad, and indicates an off-by-one error.";
        break;
      }
      if (write_buffer_.size() > static_cast<uint64_t>(bytesleft))
        flags |= MSG_MORE;
      ssize_t ret = ::send(fd_, p, bytesleft, flags);
      if (ret < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
          continue;
        }
        Abort();
        throw runtime_error(StringPrintf("send() returned error: %s", strerror(errno)));
      }
      p += ret;
    }
    write_buffer_.pop_front();
  }
}

bool Socket::Poll(int timeout, uint16_t events) const {
  if (!fd_)
    throw runtime_error("poll() on closed socket");
  Timestamp start_time;
  uint32_t attempts = 0;
  while (true) {
    int timeleft = timeout - (Timestamp::Now() - start_time.ms());
    if (timeout >= 0) {
      if (timeleft <= 0 && attempts)
        return false;
      if (timeleft < 0) {
        // An initial timeout of 0 will probably result in less than 0 ms left by the time this happens. Allow immediate
        // timeout to work.
        timeleft = 0;
      }
    } else {
      timeleft = -1;
    }
    struct pollfd fds[1];
    fds[0].fd = fd_;
    fds[0].events = events;
    int ret = poll(fds, 1, timeleft);
    if (ret > 0) {
      if (fds[0].revents & POLLERR || fds[0].revents & POLLHUP)
        return false;
      if (fds[0].revents & POLLNVAL) {
        LOG(ERROR) << "poll() returned POLLNVAL";
        return false;
      }
#ifdef _GNU_SOURCE
      if (fds[0].revents & POLLRDHUP) {
        LOG(WARNING) << "poll() returned POLLRDHUP";
        return false;
      }
#endif
      return true;
    }
    attempts++;
    if (ret <= 0) {
      if (ret == 0 || errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        continue;
      throw runtime_error(StringPrintf("Error polling listening connection: %s", strerror(errno)));
    }
  }
}

uint64_t Socket::Read(int timeout) {
  if (!fd_) {
    // Not connected
    return 0;
  }
  scoped_array<char> buf(new char[4096]);
  uint64_t bytes_read = 0;
  while (PollRead(timeout)) {
    ssize_t ret = recv(fd_, buf.get(), 4096, 0);
    if (ret < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        continue;
      Close();
      throw runtime_error(StringPrintf("recv() returned error: %s", strerror(errno)));
    }
    if (ret == 0) {
      Abort();
      break;
    }
    bytes_read += ret;
    read_buffer_.CopyFrom(buf.get(), ret);
    break;
  }
  return bytes_read;
}

void Socket::Connect(Address addr, int timeout) {
  if (fd_) {
    // Close any already open connections
    Close();
  }
  if ((fd_ = socket(addr.address_.ss_family, SOCK_STREAM, 0)) <= 0)
    throw runtime_error(StringPrintf("Can't create socket: %s", strerror(errno)));
  SetNonblocking(true);

  while (::connect(fd_, reinterpret_cast<struct sockaddr *>(&addr.address_), sizeof(addr.address_)) < 0) {
    if (errno == EINPROGRESS || errno == EINTR) {
      // Non-blocking connect
      if (!PollWrite(1000)) {
        throw runtime_error(StringPrintf("Can't connect to %s: timeout", addr.AddressToString().c_str()));
      } else {
        int optval;
        socklen_t optlen = sizeof(optval);
        if (getsockopt(fd_, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0 || optval != 0)
          throw runtime_error(StringPrintf("Can't connect to %s: %s", addr.AddressToString().c_str(), strerror(optval)));
        continue;
      }
    } else {
      LOG(WARNING) << StringPrintf("Can't connect to %s: %s", addr.AddressToString().c_str(), strerror(errno));
      throw runtime_error(StringPrintf("Can't connect to %s: %s", addr.AddressToString().c_str(), strerror(errno)));
    }
    throw runtime_error("Not connected");
  }
  socklen_t addrlen = sizeof(addr.address_);
  if (getsockname(fd_, reinterpret_cast<struct sockaddr *>(&local_.address_), &addrlen) != 0) {
    local_ = Address();
    LOG(WARNING) << "getsockname() failed, can't get local ip and port: " << strerror(errno);
  }

  remote_ = addr;
  VLOG(1) << "Connected to " << remote_.AddressToString() << " from " << local_.AddressToString();
}

void Socket::Connect(const string &address, uint16_t port, int timeout) {
  vector<Address> addrs = Resolve(address.c_str());
  if (!addrs.size())
    throw runtime_error(StringPrintf("No addresses found for %s", address.c_str()));

  for (Address &addr : addrs) {
    addr.set_port(port);
    try {
      Connect(addr, timeout);
      return;
    } catch (runtime_error e) {
      // This address failed, try another one
    }
  }
  throw runtime_error(StringPrintf("Failed to connect to %s:%u", address.c_str(), port));
}

inline bool Socket::PollRead(int timeout) const {
  return Poll(timeout, POLLIN | POLLPRI);
}

inline bool Socket::PollWrite(int timeout) const {
  return Poll(timeout, POLLOUT);
}

string Socket::Hostname() {
  char hostname[4096] = {0};
  gethostname(hostname, 4095);
  return string(hostname);
}



Socket::Address::Address() {
  Clear();
}

Socket::Address::Address(uint32_t address, uint16_t port) {
  Clear();
  sockaddr_v4()->sin_family = AF_INET;
  sockaddr_v4()->sin_addr.s_addr = htonl(address);
  sockaddr_v4()->sin_port = htons(port);
}

Socket::Address::Address(const string &address, uint16_t port) {
  Clear();
  FromString(address);
  sockaddr_v4()->sin_port = htons(port);
}

Socket::Address::Address(const string &serverport) {
  Clear();
  FromString(serverport);
}

void Socket::Address::Clear() {
  memset(&address_, 0, sizeof(address_));
}

bool Socket::Address::valid() const {
  switch (address_.ss_family) {
    case AF_INET:
      if (const_sockaddr_v4()->sin_port == 0)
        return false;
      if (const_sockaddr_v4()->sin_addr.s_addr == 0)
        return false;
      return true;
    case AF_INET6:
      if (const_sockaddr_v6()->sin6_port == 0)
        return false;
      if (memcmp(&const_sockaddr_v6()->sin6_addr, &in6addr_any, sizeof(in6addr_any)) == 0)
        return false;
      return true;
    default:
      LOG(ERROR) << "No address type defined for socket";
      return false;
  }
}

void Socket::Address::FromString(const string &addr) {
  address_.ss_family = AF_INET;
  int num_colons = std::count(addr.begin(), addr.end(), ':');
  if (num_colons == 0) {
    // IPv4 address without port
    if (inet_pton(address_.ss_family, addr.c_str(), &sockaddr_v4()->sin_addr) <= 0) {
      LOG(WARNING) << "Invalid IP address " << addr;
      Clear();
      return;
    }
  } else if (num_colons == 1) {
    // IPv4 address with port
    size_t pos = addr.find_last_of(":");
    string host = addr.substr(0, pos);
    if (inet_pton(address_.ss_family, host.c_str(), &sockaddr_v4()->sin_addr) <= 0) {
      LOG(WARNING) << "Invalid IP address:port " << addr;
      Clear();
      return;
    }
    try {
      sockaddr_v4()->sin_port = htons(lexical_cast<uint16_t>(addr.substr(pos + 1)));
    } catch (exception) {
      LOG(WARNING) << "Invalid port " << addr.substr(pos + 1);
      Clear();
      return;
    }
  } else {
    // IPv6 address, perhaps with port
    address_.ss_family = AF_INET6;
    if (addr[0] == '[') {
      size_t pos1 = addr.find_last_of("]");
      string host = addr.substr(1, pos1 - 1);
      if (inet_pton(address_.ss_family, host.c_str(), &sockaddr_v6()->sin6_addr) <= 0) {
        LOG(WARNING) << "Invalid IPv6 [address] " << addr;
        Clear();
        return;
      }
      size_t pos2 = addr.find_last_of(":");
      if (pos2 > pos1) {
        // Has a port
        try {
          sockaddr_v6()->sin6_port = htons(lexical_cast<uint16_t>(addr.substr(pos2 + 1)));
        } catch (exception) {
          LOG(WARNING) << "Invalid IPv6 port " << addr.substr(pos2 + 1);
          Clear();
          return;
        }
      }
    } else {
      if (inet_pton(address_.ss_family, addr.c_str(), &sockaddr_v6()->sin6_addr) <= 0) {
        // Should not have a port without [] around the address.
        LOG(WARNING) << "Invalid IPv6 address " << addr;
        Clear();
        return;
      }
    }
  }
}

string Socket::Address::ToString() const {
  if (v4())
    return StringPrintf("%s:%u", AddressToString().c_str(), ntohs(const_sockaddr_v4()->sin_port));
  else if (v6())
    return StringPrintf("[%s]:%u", AddressToString().c_str(), ntohs(const_sockaddr_v6()->sin6_port));
  else
    throw runtime_error("No address type defined for socket in ToString()");
}

string Socket::Address::AddressToString() const {
  char str[INET6_ADDRSTRLEN + 1] = {0};
  if (v4())
    return inet_ntop(address_.ss_family, &const_sockaddr_v4()->sin_addr, str, INET6_ADDRSTRLEN);
  else if (v6())
    return inet_ntop(address_.ss_family, &const_sockaddr_v6()->sin6_addr, str, INET6_ADDRSTRLEN);
  else
    throw runtime_error("No address type defined for socket in AddressToString()");
}

}  // namespace openinstrument
