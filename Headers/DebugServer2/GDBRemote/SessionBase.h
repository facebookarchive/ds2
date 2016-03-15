//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_GDBRemote_SessionBase_h
#define __DebugServer2_GDBRemote_SessionBase_h

#include "DebugServer2/GDBRemote/PacketProcessor.h"
#include "DebugServer2/GDBRemote/ProtocolInterpreter.h"
#include "DebugServer2/GDBRemote/Types.h"
#include "DebugServer2/Host/Channel.h"

namespace ds2 {
namespace GDBRemote {

class SessionDelegate;

class SessionBase : public ProtocolHandler {
private:
  Host::Channel *_channel;
  PacketProcessor _processor;
  ProtocolInterpreter _interpreter;

protected:
  SessionDelegate *_delegate;
  bool _ackmode;

public:
  SessionBase();
  virtual ~SessionBase();

public:
  virtual CompatibilityMode mode() const = 0;

public:
  inline void setDelegate(SessionDelegate *delegate) { _delegate = delegate; }
  inline SessionDelegate *delegate() const {
    return const_cast<SessionBase *>(this)->_delegate;
  }

public:
  bool create(Host::Channel *channel);

public:
  bool receive(bool cooked);
  bool parse(std::string const &data);

public:
  bool send(std::string const &data, bool escaped = false);

protected:
  bool sendACK();
  bool sendNAK();
  inline bool sendOK() { return send("OK"); }
  bool sendError(ErrorCode code);

public:
  inline bool getAckMode() { return _ackmode; }

protected:
  inline void setAckMode(bool enabled) { _ackmode = enabled; }

public:
  inline ProtocolInterpreter &interpreter() const {
    return const_cast<SessionBase *>(this)->_interpreter;
  }

protected:
  friend class ProtocolInterpreter;
  virtual bool onACK();
  virtual bool onNAK();
  virtual bool onCommandReceived(bool valid);
  virtual void onInvalidData(std::string const &data);
};
}
}

#endif // !__DebugServer2_GDBRemote_Session_h
