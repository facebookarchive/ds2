//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Host_QueueChannel_h
#define __DebugServer2_Host_QueueChannel_h

#include "DebugServer2/Host/Channel.h"
#include "DebugServer2/MessageQueue.h"
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
  virtual ~QueueChannel();

public:
  inline Channel *remote() const {
    return const_cast<QueueChannel *>(this)->_remote;
  }
  inline MessageQueue &queue() const {
    return const_cast<QueueChannel *>(this)->_queue;
  }

public:
  virtual void close();

public:
  virtual bool connected() const;

public:
  virtual bool wait(int ms = -1);

public:
  virtual ssize_t send(void const *buffer, size_t length);
  virtual ssize_t receive(void *buffer, size_t length);

public:
  virtual bool receive(std::string &buffer);
};
}
}

#endif // !__DebugServer2_Host_QueueChannel_h
