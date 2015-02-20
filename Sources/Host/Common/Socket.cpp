//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/Socket.h"
#if defined(_WIN32)
#include "DebugServer2/Host/Windows/ExtraWrappers.h"
#endif

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#define SOCK_ERRNO WSAGetLastError()
#define SOCK_WOULDBLOCK WSAEWOULDBLOCK
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#define SOCK_ERRNO errno
#define SOCK_WOULDBLOCK EAGAIN
#endif

#include <cstring>

namespace ds2 {
namespace Host {

Socket::Socket()
    : _handle(INVALID_SOCKET), _state(kStateInvalid), _lastError(0) {}

Socket::Socket(SOCKET handle)
    : _handle(handle), _state(kStateConnected), _lastError(0) {}

Socket::~Socket() { close(); }

bool Socket::create() {
  if (valid())
    return false;

  _handle = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (_handle < 0) {
    _lastError = SOCK_ERRNO;
  }

#if !defined(_WIN32)
  int flags = ::fcntl(_handle, F_GETFD) | FD_CLOEXEC;
  ::fcntl(_handle, F_SETFD, flags);
#endif

  return valid();
}

void Socket::close() {
  if (!valid())
    return;

#if defined(_WIN32)
  ::closesocket(_handle);
#else
  ::close(_handle);
#endif

  _state = kStateInvalid;
  _handle = INVALID_SOCKET;
  _lastError = 0;
}

bool Socket::listen(char const *address, uint16_t port) {
  if (!valid())
    return false;

  if (listening() || connected())
    return false;

  struct linger linger;
  linger.l_onoff = 1;
  linger.l_linger = 0;
  ::setsockopt(_handle, SOL_SOCKET, SO_LINGER,
               reinterpret_cast<char *>(&linger), sizeof(linger));

  struct sockaddr_in sin;
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = (address == nullptr || address[0] == '\0')
                            ? INADDR_ANY
                            : ::inet_addr(address);
  sin.sin_port = htons(port);

  if (::bind(_handle, reinterpret_cast<struct sockaddr *>(&sin), sizeof(sin)) <
      0) {
    _lastError = SOCK_ERRNO;
    return false;
  }

  if (::listen(_handle, 1) < 0) {
    _lastError = SOCK_ERRNO;
    return false;
  }

  _state = kStateListening;
  return true;
}

Socket *Socket::accept() {
  if (!listening())
    return nullptr;

  struct sockaddr_in sin;
  socklen_t sinlen = sizeof(sin);
  SOCKET handle;

  handle =
      ::accept(_handle, reinterpret_cast<struct sockaddr *>(&sin), &sinlen);
  if (handle == INVALID_SOCKET) {
    _lastError = SOCK_ERRNO;
    return nullptr;
  }

#if !defined(_WIN32)
  int flags = ::fcntl(handle, F_GETFD) | FD_CLOEXEC;
  ::fcntl(handle, F_SETFD, flags);
#endif

  Socket *client = new Socket(handle);
  client->setNonBlocking();
  return client;
}

bool Socket::setNonBlocking() {
  if (!connected())
    return false;

#if defined(_WIN32)
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
  if (!connected())
    return -1;

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
  if (!connected())
    return -1;

  if (length == 0)
    return 0;

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
  if (!valid())
    return false;

#if defined(_WIN32)
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
  int nfds = poll(&pfd, 1, ms);
  return (nfds == 1 && (pfd.revents & POLLIN) != 0);
#endif
}

std::string Socket::error() const {
#if defined(_WIN32)
  // 128 bytes is enough for "error " + "0x00000000"
  char buf[128];
  ds2_snprintf(buf, sizeof(buf), "error %#x", _lastError);
  return std::string(buf);
#else
  return strerror(_lastError);
#endif
}

uint16_t Socket::port() const {
  if (_handle == INVALID_SOCKET)
    return 0;

  struct sockaddr_storage ss;
  socklen_t sslen = sizeof(ss);
  if (::getsockname(_handle, reinterpret_cast<struct sockaddr *>(&ss), &sslen) <
      0)
    return 0;

  switch (ss.ss_family) {
  case AF_INET:
    return ntohs(reinterpret_cast<struct sockaddr_in *>(&ss)->sin_port);
  case AF_INET6:
    return ntohs(reinterpret_cast<struct sockaddr_in6 *>(&ss)->sin6_port);
  default:
    break;
  }
  return 0;
}
}
}
