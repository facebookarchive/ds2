//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Host_Socket_h
#define __DebugServer2_Host_Socket_h

#include "DebugServer2/Host/Channel.h"

#if defined(OS_WIN32)
#include <winsock2.h>
#endif

namespace ds2 {
namespace Host {

class Socket : public Channel {
private:
#if defined(OS_WIN32)
  typedef int socklen_t;
#elif defined(OS_LINUX) || defined(OS_FREEBSD)
  typedef int SOCKET;
  static SOCKET const INVALID_SOCKET = -1;
#endif

protected:
  enum State { kStateInvalid = 0, kStateListening = 1, kStateConnected = 2 };

protected:
  SOCKET _handle;
  State _state;
  int _lastError;

private:
  Socket(SOCKET handle);

public:
  Socket();
  ~Socket() override;

public:
  void close() override;

public:
  inline bool valid() const { return (_handle != INVALID_SOCKET); }

public:
  inline bool listening() const { return (_state == kStateListening); }
  inline bool connected() const override { return (_state == kStateConnected); }

protected:
  bool create(int af);

public:
  bool listen(std::string const &address, std::string const &port);
  inline bool listen(std::string const &port) { return listen(nullptr, port); }
  Socket *accept();
  bool connect(std::string const &host, std::string const &port);

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
}
}

#endif // !__DebugServer2_Host_Socket_h
