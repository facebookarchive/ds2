//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/Channel.h"

namespace ds2 {
namespace Host {

bool Channel::send(std::string const &buffer) {
  if (!connected())
    return false;

  return send(&buffer[0], buffer.size()) == static_cast<ssize_t>(buffer.size());
}

bool Channel::receive(std::string &buffer) {
  if (!connected())
    return false;

  buffer.clear();

  size_t total = 0;
  static size_t const chunk = 1024;
  for (;;) {
    size_t size = buffer.size();
    buffer.resize(size + chunk);
    ssize_t nrecvd = receive(&buffer[size], chunk);
    if (nrecvd == 0)
      break;
    buffer.resize(size + nrecvd);

    total += nrecvd;
  }
  buffer.resize(total);
  return !buffer.empty();
}
} // namespace Host
} // namespace ds2
