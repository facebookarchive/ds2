//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Core/SessionThread.h"

using ds2::GDBRemote::Session;
using ds2::Host::QueueChannel;

SessionThread::SessionThread(QueueChannel *channel, Session *session)
    : _channel(channel), _session(session) {
  _pp.setDelegate(this);
}

SessionThread::~SessionThread() { _thread.join(); }

void SessionThread::start() {
  _thread = std::thread(&SessionThread::run, this);
}

void SessionThread::run() {
  //
  // Wait for a message and pass down to the packet processor.
  //
  while (_channel->connected()) {
    std::string data;

    if (!_channel->remote()->wait())
      break;

    if (!_channel->remote()->receive(data))
      break;

    _pp.parse(data);
  }

  _channel->close();
}

void SessionThread::onPacketData(std::string const &data, bool valid) {
  if (data.length() == 1 && data[0] == '\x03') {
    //
    // Interrupt process, this is the highest priority message
    // we can receive, as such we must deliver it to the delegate
    // directly. Because of the nature of this message, the message
    // queue must be discarded.
    //
    // Note that Interrupt is the only message that can be used
    // in a different thread, all other messages must be processed
    // on the main thread due to restrictions imposed by the interaction
    // of Linux threading and ptrace(2) system call.
    //
    _channel->queue().clear(false);
    _session->interpreter().onPacketData(data, valid);
  } else {
    if (_session->getAckMode() && !valid) {
      //
      // In case of invalid message, we forward to the session directly
      // so that it can act as necessary, calling onPacketData in another
      // thread is safe when valid is false as there's no interaction
      // with the system in such a case.
      //
      _session->interpreter().onPacketData(data, valid);
    } else {
      //
      // This is a normal valid message, enqueue it, the main thread will
      // activate to fetch the message and process it.
      //
      _channel->queue().put(data);
    }
  }
}

void SessionThread::onInvalidData(std::string const &data) {
  //
  // Forward to the session's interpreter.
  //
  _session->interpreter().onInvalidData(data);
}
