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

#include "DebugServer2/Core/MessageQueue.h"
#include "DebugServer2/Host/Channel.h"
#include "DebugServer2/Types.h"

#include <string>

namespace ds2 {
namespace Host {

class QueueChannel : public Channel {
public:
  Channel *_remote;
  MessageQueue _queue;

public:
  QueueChannel(Channel *remote);
  ~QueueChannel() override;

public:
  inline Channel *remote() const {
    return const_cast<QueueChannel *>(this)->_remote;
  }
  inline MessageQueue &queue() const {
    return const_cast<QueueChannel *>(this)->_queue;
  }

public:
  void close() override;

public:
  bool connected() const override;

public:
  bool wait(int ms = -1) override;

public:
  ssize_t send(void const *buffer, size_t length) override;
  ssize_t receive(void *buffer, size_t length) override;

public:
  bool receive(std::string &buffer) override;
};
} // namespace Host
} // namespace ds2
