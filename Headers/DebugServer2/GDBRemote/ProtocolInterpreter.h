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

#include "DebugServer2/GDBRemote/PacketProcessor.h"

namespace ds2 {
namespace GDBRemote {

class SessionBase;
struct ProtocolHandler;

class ProtocolInterpreter : public PacketProcessorDelegate {
public:
  struct Handler {
    typedef std::vector<Handler> Collection;
    typedef void (ProtocolHandler::*Callback)(Handler const &handler,
                                              std::string const &data);

    enum Mode { kModeEquals, kModeStartsWith };

    Mode mode;
    std::string command;
    ProtocolHandler *handler;
    Callback callback;

    int compare(std::string const &command) const;
  };

private:
  SessionBase *_session;
  Handler::Collection _handlers;
  std::vector<std::string> _lastCommands;

public:
  ProtocolInterpreter();

public:
  inline void setSession(SessionBase *session) { _session = session; }
  inline SessionBase *session() const {
    return const_cast<ProtocolInterpreter *>(this)->_session;
  }

public:
  void onCommand(std::string const &command, std::string const &arguments);

public:
  bool registerHandler(Handler const &handler);

  template <typename CallbackType>
  inline bool
  registerHandler(Handler::Mode const &mode, std::string const &command,
                  ProtocolHandler *handler, CallbackType const &callback) {
    Handler _handler = {mode, command, handler, (Handler::Callback)callback};
    return registerHandler(_handler);
  }

private:
  Handler const *findHandler(std::string const &command,
                             size_t &commandLength) const;

public:
  void onPacketData(std::string const &data, bool valid) override;
  void onInvalidData(std::string const &data) override;

public:
  inline std::vector<std::string> const &lastCommands() const {
    return _lastCommands;
  }
  inline void clearLastCommands() { _lastCommands.clear(); }
};

struct ProtocolHandler {};
} // namespace GDBRemote
} // namespace ds2
