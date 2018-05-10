//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/Socket.h"
#include "DebugServer2/Utils/Log.h"
#include "DebugServer2/Utils/String.h"
#include "DebugServer2/Utils/Stringify.h"
#if defined(OS_WIN32)
#include "DebugServer2/Host/Windows/ExtraWrappers.h"
#endif

#if defined(OS_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#define SOCK_ERRNO WSAGetLastError()
#define SOCK_WOULDBLOCK WSAEWOULDBLOCK
#define SOCK_NAMETOOLONG WSAENAMETOOLONG
#define SOCK_ERRNO_STRINGIFY Stringify::WSAError
#elif defined(OS_POSIX)
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/un.h>
#include <unistd.h>
#define SOCK_ERRNO errno
#define SOCK_WOULDBLOCK EAGAIN
#define SOCK_NAMETOOLONG ENAMETOOLONG
#define SOCK_ERRNO_STRINGIFY Stringify::Errno
#endif

#include <algorithm>
#include <cstddef>
#include <cstring>

using ds2::Utils::Stringify;

namespace ds2 {
namespace Host {

Socket::Socket()
    : _handle(INVALID_SOCKET), _state(State::Invalid), _lastError(0) {}

Socket::Socket(SOCKET handle)
    : _handle(handle), _state(State::Connected), _lastError(0) {
#if defined(OS_POSIX)
  ::fcntl(_handle, F_SETFD, FD_CLOEXEC);
  ::fcntl(_handle, F_SETFL, O_NONBLOCK);
#endif
}

Socket::~Socket() { close(); }

bool Socket::create(int af) {
  if (valid()) {
    return false;
  }

  _handle = ::socket(af, SOCK_STREAM, 0);
  if (_handle == INVALID_SOCKET) {
    _lastError = SOCK_ERRNO;
    return false;
  }
#if defined(OS_POSIX)
  fcntl(F_SETFD, _handle, FD_CLOEXEC);
#endif

  // On most Linux systems, IPV6_V6ONLY is off by default, but on some systems
  // it's on, so turn it off to be able to receive both IPv6 and IPv4
  // connections when we listen on IN6ADDR_ANY.
  if (af == AF_INET6) {
    int no = 0;
    int res = setsockopt(_handle, IPPROTO_IPV6, IPV6_V6ONLY,
                         reinterpret_cast<char *>(&no), sizeof(no));
    if (res != 0) {
      _lastError = SOCK_ERRNO;
      return false;
    }
  }

#if !defined(OS_WIN32)
  int flags = ::fcntl(_handle, F_GETFD) | FD_CLOEXEC;
  ::fcntl(_handle, F_SETFD, flags);
#endif

  return valid();
}

void Socket::close() {
  if (!valid()) {
    return;
  }

#if defined(OS_WIN32)
  ::closesocket(_handle);
#else
  ::close(_handle);
#endif

  _state = State::Invalid;
  _handle = INVALID_SOCKET;
  _lastError = 0;
}

bool Socket::listen(std::string const &address, std::string const &port) {
  if (listening() || connected()) {
    return false;
  }

  struct addrinfo hints, *result;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_protocol = IPPROTO_TCP;

  // On most system, the resolver will return an IPv6 for "localhost". This is
  // not what most users expect, as they might have their server listening on
  // "localhost", and their client trying to connect to "127.0.0.1". This will
  // have the server socket accepting only IPv6 connections, and the client
  // socket trying to connect to an IPv4 address.
  // While this is true for all addresses (not just "localhost"), "localhost"
  // is the one that causes issues most frequently. Force ai_family to AF_INET
  // here so we don't fail.
  if (address == "localhost") {
    hints.ai_family = AF_INET;
  }

  int res = ::getaddrinfo(address.c_str(), port.c_str(), &hints, &result);
  if (res != 0) {
    _lastError = SOCK_ERRNO;
    return false;
  }

  if (!create(result->ai_family)) {
    return false;
  }

  // Disable socket lingering so the process can die quickly when we exit.
  struct linger linger;
  linger.l_onoff = 0;
  linger.l_linger = 0;
  if (::setsockopt(_handle, SOL_SOCKET, SO_LINGER,
                   reinterpret_cast<char *>(&linger), sizeof(linger)) == -1) {
    DS2LOG(Warning,
           "unable to disable SO_LINGER on the server socket, errno=%s",
           SOCK_ERRNO_STRINGIFY(SOCK_ERRNO));
  }

  // Enable SO_REUSEADDR so we don't crash when trying to reuse a port after a
  // previous instance of ds2 exits.
  int enabled = 1;
  if (::setsockopt(_handle, SOL_SOCKET, SO_REUSEADDR,
                   reinterpret_cast<char *>(&enabled), sizeof(enabled)) == -1) {
    DS2LOG(Warning,
           "unable to enable SO_REUSEADDR on the server socket, errno=%s",
           SOCK_ERRNO_STRINGIFY(SOCK_ERRNO));
  }

  res = ::bind(_handle, result->ai_addr, result->ai_addrlen);
  freeaddrinfo(result);
  if (res < 0) {
    _lastError = SOCK_ERRNO;
    return false;
  }

  if (::listen(_handle, 1) < 0) {
    _lastError = SOCK_ERRNO;
    return false;
  }

  _state = State::Listening;
  return true;
}

#if defined(OS_POSIX)
bool Socket::listen(std::string const &path, bool abstract) {
#if !defined(OS_LINUX)
  // Abstract UNIX sockets are supported only on Linux.
  DS2ASSERT(!abstract);
#endif

  if (listening() || connected()) {
    return false;
  }

  if (!create(AF_UNIX)) {
    return false;
  }

  struct sockaddr_un sun;
  ::memset(&sun, 0, sizeof(sun));
  sun.sun_family = AF_UNIX;

  if (abstract) {
    // Abstract UNIX sockets are a Linux-only construct where the socket
    // doesn't actually exist on the filesystem, and the given "path" is used
    // as a generic identifier for the socket. Abstract UNIX sockets are
    // identified by `sun_path` starting with a null character.
    if (path.length() > sizeof(sun.sun_path) - 1) {
      _lastError = SOCK_NAMETOOLONG;
      return false;
    }
    ::strncpy(sun.sun_path + 1, path.c_str(), sizeof(sun.sun_path) - 1);
  } else {
    if (path.length() > sizeof(sun.sun_path)) {
      _lastError = SOCK_NAMETOOLONG;
      return false;
    }
    ::strncpy(sun.sun_path, path.c_str(), sizeof(sun.sun_path));
  }

  size_t len = offsetof(struct sockaddr_un, sun_path) + path.length() +
               (abstract ? sizeof(char) : 0);
  int res = ::bind(_handle, reinterpret_cast<struct sockaddr *>(&sun), len);
  if (res < 0) {
    _lastError = SOCK_ERRNO;
    return false;
  }

  if (::listen(_handle, 1) < 0) {
    _lastError = SOCK_ERRNO;
    return false;
  }

  _state = State::Listening;
  return true;
}
#endif

std::unique_ptr<Socket> Socket::accept() {
  if (!listening()) {
    return nullptr;
  }

  struct sockaddr_storage ss;
  socklen_t sslen = sizeof(ss);
  SOCKET handle;

  handle = ::accept(_handle, reinterpret_cast<struct sockaddr *>(&ss), &sslen);
  if (handle == INVALID_SOCKET) {
    _lastError = SOCK_ERRNO;
    return nullptr;
  }

#if !defined(OS_WIN32)
  int flags = ::fcntl(handle, F_GETFD) | FD_CLOEXEC;
  ::fcntl(handle, F_SETFD, flags);
#endif

  auto client = make_protected_unique(handle);
  client->setNonBlocking();
  return client;
}

bool Socket::connect(std::string const &host, std::string const &port) {
  struct addrinfo hints;
  struct addrinfo *result, *results;

  ::memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  // See Socket::listen.
  if (host == "localhost") {
    hints.ai_family = AF_INET;
  }

  int res = ::getaddrinfo(host.c_str(), port.c_str(), &hints, &results);
  if (res != 0) {
    _lastError = SOCK_ERRNO;
    return false;
  }

  SOCKET handle = INVALID_SOCKET;
  for (result = results; result != NULL; result = result->ai_next) {
    if (!create(result->ai_family)) {
      continue;
    }
    handle = ::connect(_handle, result->ai_addr, result->ai_addrlen);
    if (handle == INVALID_SOCKET) {
      close();
      continue;
    } else {
      break;
    }
  }
  if (handle == INVALID_SOCKET) {
    _lastError = SOCK_ERRNO;
    return false;
  }

  ::freeaddrinfo(results);

  _state = State::Connected;
  setNonBlocking();

  return true;
}

bool Socket::setNonBlocking() {
  if (!connected()) {
    return false;
  }

#if defined(OS_WIN32)
  u_long set = 1;
  if (::ioctlsocket(_handle, FIONBIO, &set) == SOCKET_ERROR) {
    _lastError = SOCK_ERRNO;
    return false;
  }
#else
  int flags = ::fcntl(_handle, F_GETFL, 0) | O_NONBLOCK;
  if (::fcntl(_handle, F_SETFL, flags) < 0) {
    _lastError = SOCK_ERRNO;
    return false;
  }
#endif
  return true;
}

ssize_t Socket::send(void const *buffer, size_t length) {
  if (!connected()) {
    return -1;
  }

  ssize_t nsent =
      ::send(_handle, reinterpret_cast<const char *>(buffer), length, 0);
  if (nsent < 0) {
    int err = SOCK_ERRNO;
    if (err != SOCK_WOULDBLOCK) {
      close();
      _lastError = err;
    }
    return -1;
  }
  return nsent;
}

ssize_t Socket::receive(void *buffer, size_t length) {
  if (!connected()) {
    return -1;
  }

  if (length == 0) {
    return 0;
  }

  ssize_t nrecvd = ::recv(_handle, reinterpret_cast<char *>(buffer), length, 0);
  if (nrecvd <= 0) {
    int err = SOCK_ERRNO;
    if (err != SOCK_WOULDBLOCK) {
      close();
      _lastError = err;
    }
    nrecvd = 0;
  }

  return nrecvd;
}

bool Socket::wait(int ms) {
  if (!valid()) {
    return false;
  }

#if defined(OS_WIN32)
  fd_set fds;
  struct timeval tv, *ptv;
  if (ms < 0) {
    ptv = nullptr;
  } else {
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    ptv = &tv;
  }

  FD_ZERO(&fds);
  FD_SET(_handle, &fds);
  int nfds = ::select(_handle + 1, &fds, nullptr, nullptr, ptv);
  return (nfds == 1);
#else
  struct pollfd pfd;
  pfd.fd = _handle;
  pfd.events = POLLIN;
  int nfds = ::poll(&pfd, 1, ms);
  return (nfds == 1 && (pfd.revents & POLLIN) != 0);
#endif
}

std::string Socket::error() const {
#if defined(OS_WIN32)
  // 128 bytes is enough for "error " + "0x00000000"
  char buf[128];
  ds2::Utils::SNPrintf(buf, sizeof(buf), "error %#x", _lastError);
  return std::string(buf);
#else
  return strerror(_lastError);
#endif
}

bool Socket::getSocketInfo(struct sockaddr_storage *ss) const {
  if (_handle == INVALID_SOCKET) {
    return false;
  }

  socklen_t sslen = sizeof(struct sockaddr_storage);
  auto func = listening() ? ::getsockname : ::getpeername;
  return func(_handle, reinterpret_cast<struct sockaddr *>(ss), &sslen) >= 0;
}

std::string Socket::address() const {
  struct sockaddr_storage ss;
  if (!getSocketInfo(&ss)) {
    return std::string();
  }

  char addressStr[std::max(INET_ADDRSTRLEN, INET6_ADDRSTRLEN)];
  switch (ss.ss_family) {
  case AF_INET:
    inet_ntop(AF_INET,
              &reinterpret_cast<struct sockaddr_in *>(&ss)->sin_addr.s_addr,
              addressStr, sizeof(addressStr));
    break;
  case AF_INET6:
    inet_ntop(AF_INET6,
              &reinterpret_cast<struct sockaddr_in6 *>(&ss)->sin6_addr.s6_addr,
              addressStr, sizeof(addressStr));
    break;
  default:
    DS2BUG("unknown socket family: %u", (unsigned int)ss.ss_family);
  }
  return addressStr;
}

std::string Socket::port() const {
  struct sockaddr_storage ss;
  if (!getSocketInfo(&ss)) {
    return std::string();
  }

  switch (ss.ss_family) {
  case AF_INET:
    return ds2::Utils::ToString(
        ntohs(reinterpret_cast<struct sockaddr_in *>(&ss)->sin_port));
  case AF_INET6:
    return ds2::Utils::ToString(
        ntohs(reinterpret_cast<struct sockaddr_in6 *>(&ss)->sin6_port));
  default:
    DS2BUG("unknown socket family: %u", (unsigned int)ss.ss_family);
  }
}
} // namespace Host
} // namespace ds2
