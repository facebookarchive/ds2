//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/QueueChannel.h"

#include <algorithm>
#include <cstring>

namespace ds2 {
namespace Host {

QueueChannel::QueueChannel(Channel *remote) : _remote(remote) {}

QueueChannel::~QueueChannel() { close(); }

void QueueChannel::close() {
  if (_remote != nullptr) {
    _remote = nullptr;
    _queue.clear(true);
  }
}

bool QueueChannel::connected() const {
  return _remote != nullptr && _remote->connected();
}

bool QueueChannel::wait(int ms) {
  _queue.wait(ms);
  return true;
}

ssize_t QueueChannel::send(void const *buffer, size_t length) {
  // Forward to the remote
  if (!connected())
    return -1;

  return _remote->send(buffer, length);
}

//
// This method is for compatibility, the code should always call
// receive(std::string&) when using a QueueChannel, which is the
// case for DebugServer2.
//
ssize_t QueueChannel::receive(void *buffer, size_t length) {
  std::string strbuf;
  if (!receive(strbuf))
    return false;

  length = std::min(length, strbuf.size());
  std::memcpy(buffer, strbuf.data(), length);
  return length;
}

bool QueueChannel::receive(std::string &buffer) {
  if (!connected())
    return false;

  buffer = _queue.get(0);
  return true;
}
} // namespace Host
} // namespace ds2
