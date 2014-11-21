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

#if defined(_WIN32)
#include <Winsock2.h>
#endif

namespace ds2 {
namespace Host {

class Socket : public Channel {
private:
#ifdef _WIN32
  typedef int socklen_t;
#else
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
  ~Socket();

public:
  void close();

public:
  inline bool valid() const { return (_handle != INVALID_SOCKET); }

public:
  inline bool listening() const { return (_state == kStateListening); }
  inline virtual bool connected() const { return (_state == kStateConnected); }

public:
  bool create();

public:
  bool listen(char const *address, uint16_t port);
  inline bool listen(uint16_t port) { return listen(nullptr, port); }
  Socket *accept();

public:
  uint16_t port() const;

public:
  std::string error() const;

public:
  bool wait(int ms = -1);

public:
  bool setNonBlocking();

public:
  ssize_t send(void const *buffer, size_t length);
  ssize_t receive(void *buffer, size_t length);
};
}
}

#endif // !__DebugServer2_Host_Socket_h
