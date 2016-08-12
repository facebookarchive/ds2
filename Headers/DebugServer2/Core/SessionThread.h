//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

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
