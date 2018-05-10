//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#pragma once

#include "DebugServer2/Host/Channel.h"

#include <memory>
#if defined(OS_WIN32)
#include <winsock2.h>
#elif defined(OS_POSIX)
#include <sys/socket.h>
#endif

namespace ds2 {
namespace Host {

class Socket : public Channel, public make_unique_enabler<Socket> {
private:
#if defined(OS_WIN32)
  typedef int socklen_t;
#elif defined(OS_POSIX)
  typedef int SOCKET;
  static SOCKET const INVALID_SOCKET = -1;
#endif

protected:
  enum class State { Invalid, Listening, Connected };

protected:
  SOCKET _handle;
  State _state;
  int _lastError;

public:
  Socket();
  Socket(SOCKET handle);
  ~Socket() override;

public:
  void close() override;

public:
  inline bool valid() const { return (_handle != INVALID_SOCKET); }

public:
  inline bool listening() const { return (_state == State::Listening); }
  inline bool connected() const override {
    return (_state == State::Connected);
  }

protected:
  bool create(int af);

public:
  bool listen(std::string const &address, std::string const &port);
#if defined(OS_POSIX)
  bool listen(std::string const &path, bool abstract = false);
#endif
  std::unique_ptr<Socket> accept();
  bool connect(std::string const &host, std::string const &port);

protected:
  bool getSocketInfo(struct sockaddr_storage *ss) const;

public:
  std::string address() const;
  std::string port() const;

public:
  std::string error() const;

public:
  bool wait(int ms = -1) override;

public:
  bool setNonBlocking();

public:
  ssize_t send(void const *buffer, size_t length) override;
  ssize_t receive(void *buffer, size_t length) override;
};
} // namespace Host
} // namespace ds2
