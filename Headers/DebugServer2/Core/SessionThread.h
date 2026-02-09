// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#ifndef __SessionThread_h
#define __SessionThread_h

#include "DebugServer2/GDBRemote/PacketProcessor.h"
#include "DebugServer2/GDBRemote/Session.h"
#include "DebugServer2/Host/QueueChannel.h"

#include <thread>

class SessionThread : public ds2::GDBRemote::PacketProcessorDelegate {
private:
  ds2::Host::QueueChannel *_channel;
  ds2::GDBRemote::Session *_session;
  ds2::GDBRemote::PacketProcessor _pp;
  std::thread _thread;

public:
  SessionThread(ds2::Host::QueueChannel *channel,
                ds2::GDBRemote::Session *session);
  ~SessionThread();

public:
  void start();

protected:
  void onPacketData(std::string const &data, bool valid) override;
  void onInvalidData(std::string const &data) override;

private:
  void run();
};

#endif // !__SessionThread_h
