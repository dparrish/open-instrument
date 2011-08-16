/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <vector>
#include "lib/common.h"
#include "lib/socket.h"
#include "lib/timer.h"

namespace openinstrument {

vector<Socket::Address> Socket::Resolve(const char *hostname) {
  struct addrinfo hints = {0};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo *result;
  int s = getaddrinfo(hostname, NULL, &hints, &result);
  if (s != 0)
    throw runtime_error(StringPrintf("getaddrinfo %s: %s", hostname, gai_strerror(s)));
  vector<Socket::Address> results;
  for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
    results.push_back(Address(ntohl(reinterpret_cast<struct sockaddr_in *>(rp->ai_addr)->sin_addr.s_addr), 0));
  }
  freeaddrinfo(result);

  return results;
}

void Socket::Listen(const Socket::Address &addr) {
  local_ = addr;
  if ((fd_ = ::socket(AF_INET, SOCK_STREAM, 0)) <= 0)
    throw runtime_error(StringPrintf("Can't create socket: %s", strerror(errno)));
  int on = 1;
  setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  struct sockaddr_in servaddr = {0};
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(local_.address);
  servaddr.sin_port = htons(local_.port);
  if (::bind(fd_, reinterpret_cast<struct sockaddr *>(&servaddr), sizeof(servaddr)) < 0)
    throw runtime_error(StringPrintf("Can't bind socket: %s", strerror(errno)));
  if (::listen(fd_, 10) < 0)
    throw runtime_error(StringPrintf("listen: %s", strerror(errno)));
  SetNonblocking(true);
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
  struct sockaddr_in cliaddr;
  socklen_t len = sizeof(cliaddr);
  while (true) {
    // Wait up to <timeout> ms for a connection
    if (!PollRead(timeout)) {
      if (timeout >= 0)
        return NULL;
    }

    int connfd = ::accept(fd_, reinterpret_cast<struct sockaddr *>(&cliaddr), &len);
    if (connfd <= 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        continue;
      throw runtime_error(StringPrintf("Error accepting client connection: %s", strerror(errno)));
    }

    Socket *client = new Socket(connfd);
    client->mutable_local()->address = local_.address;
    client->mutable_local()->port = local_.port;
    client->mutable_remote()->address = ntohl(cliaddr.sin_addr.s_addr);
    client->mutable_remote()->port = ntohs(cliaddr.sin_port);
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
    } else {
      timeleft = -1;
    }
    if (timeleft < 0)
      timeleft = 0;
    struct pollfd fds[1];
    fds[0].fd = fd_;
    fds[0].events = events;
    int ret = poll(fds, 1, timeleft);
    if (ret > 0)
      return true;
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

Socket::Address::Address(const string &serverport) : address(0), port(0) {
  string host;
  size_t pos = serverport.find_last_of(":");
  if (pos == string::npos) {
    // No port
    host = serverport;
  } else {
    host = serverport.substr(0, pos);
  }
  if (host.empty()) {
    LOG(WARNING) << "Invalid server host";
    return;
  }
  try {
    if (pos != string::npos && !(port = lexical_cast<uint16_t>(serverport.substr(pos + 1)))) {
      LOG(WARNING) << "Invalid server port";
      return;
    }
  } catch (exception) {
    LOG(WARNING) << "Invalid server port";
    return;
  }
  address = AddressFromString(host);
}

uint32_t Socket::Address::AddressFromString(const string &addr) {
  struct in_addr in;
  if (inet_aton(addr.c_str(), &in) == 0) {
    vector<Address> addrs = Resolve(addr.c_str());
    if (addrs.size())
      return address = addrs[0].address;
    throw runtime_error(StringPrintf("Invalid address %s", addr.c_str()));
  }
  memcpy(&address, &in, sizeof(uint32_t));
  address = ntohl(address);
  return address;
}

string Socket::Address::AddressToString(uint32_t addr) {
  struct in_addr in;
  uint32_t t = htonl(addr);
  memcpy(&in, &t, sizeof(uint32_t));
  return inet_ntoa(in);
}

void Socket::Connect(const Address &addr, int timeout) {
  if (fd_) {
    // Close any already open connections
    Close();
  }
  if ((fd_ = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
    throw runtime_error(StringPrintf("Can't create socket: %s", strerror(errno)));
  SetNonblocking(true);

  struct sockaddr_in servaddr = {0};
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(addr.address);
  servaddr.sin_port = htons(addr.port);
  if (::connect(fd_, reinterpret_cast<struct sockaddr *>(&servaddr), sizeof(servaddr)) < 0) {
    if (errno == EINPROGRESS || errno == EINTR) {
      // Non-blocking connect
      if (!PollWrite(1000)) {
        throw runtime_error(StringPrintf("Can't connect to %s: timeout", addr.ToString().c_str()));
      } else {
        int optval;
        socklen_t optlen = sizeof(optval);
        if (getsockopt(fd_, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0 || optval != 0)
          throw runtime_error(StringPrintf("Can't connect to %s: %s", addr.ToString().c_str(), strerror(optval)));
      }
    } else {
      throw runtime_error(StringPrintf("Can't connect to %s: %s", addr.ToString().c_str(), strerror(errno)));
    }
  }

  socklen_t addrlen = sizeof(servaddr);
  if (getsockname(fd_, reinterpret_cast<struct sockaddr *>(&servaddr), &addrlen) == 0) {
    local_ = Address(ntohl(servaddr.sin_addr.s_addr), ntohs(servaddr.sin_port));
  } else {
    local_ = Address();
    LOG(WARNING) << "getsockname() failed, can't get local ip and port";
  }
  remote_ = addr;
  VLOG(1) << "Connected to " << remote_.ToString() << " from " << local_.ToString();
}

void Socket::Connect(const string &address, uint16_t port, int timeout) {
  vector<Address> addrs = Resolve(address.c_str());
  if (!addrs.size())
    throw runtime_error(StringPrintf("No addresses found for %s", address.c_str()));

  BOOST_FOREACH(Address &addr, addrs) {
    addr.port = port;
    try {
      Connect(addr, timeout);
      return;
    } catch (runtime_error) {
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

}  // namespace openinstrument
