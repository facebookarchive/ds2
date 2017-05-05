//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/GDBRemote/PacketProcessor.h"
#include "DebugServer2/GDBRemote/ProtocolHelpers.h"
#include "DebugServer2/Utils/Log.h"

#include <cstdlib>

namespace ds2 {
namespace GDBRemote {

PacketProcessor::PacketProcessor()
    : _nreqs(0), _needhash(false), _delegate(nullptr) {}

bool PacketProcessor::validate() {
  if (_buffer.empty())
    return false;

  if (_buffer[0] == '+' || _buffer[0] == '-' || _buffer[0] == '\x03')
    return true;

  size_t csoff = _buffer.find('#');
  uint8_t csum =
      static_cast<uint8_t>(std::strtoul(&_buffer[csoff + 1], nullptr, 16));

  _buffer.erase(0, 1);
  _buffer.erase(csoff - 1);

  uint8_t our_csum = Checksum(_buffer);

  if (csum != our_csum)
    DS2LOG(Warning,
           "received packet %s with invalid checksum, should be %.2x, is %.2x",
           _buffer.c_str(), our_csum, csum);

  return csum == our_csum;
}

void PacketProcessor::process() {
  _delegate->onPacketData(_buffer, validate());
  _buffer.clear();
  _nreqs = 0;
}

void PacketProcessor::parse(std::string const &data) {
  size_t n = _nreqs;

  if (data.empty() || _delegate == nullptr)
    return;

  //
  // Do we need to complete a packet?
  //
  if (n != 0 && data.length() != 0) {
    _buffer += data.substr(0, n);
    if (data.length() < _nreqs) {
      _nreqs -= data.length();
      if (_nreqs != 0)
        return;
    }

    process();
  }

  //
  // First will be std::string::npos if a packet
  // has been processed.
  //
  size_t first = n;
  while (n < data.size()) {
    size_t end = std::string::npos;
    if (_needhash) {
      _needhash = false;
      goto hash;
    }

    switch (data[n]) {
    case '+':    // ACK
    case '-':    // NAK
    case '\x03': // Halt Target
      end = n + 1;
      break;

    case '$':
    hash:
      end = data.find('#', n);
      if (end++ != std::string::npos) {
        size_t avail = data.length() - end;
        if (avail < 2) {
          _buffer += data.substr(n);
          _nreqs = 2 - avail;
          return;
        } else {
          end += 2;
        }
      } else {
        _buffer += data.substr(n);
        _needhash = true;
        return;
      }
      break;

    default:
      n++;
      continue;
    }

    if (end != std::string::npos) {
      first = std::string::npos;
      _buffer += data.substr(n, end - n);
      process();
    }
    n = end;
  }

  //
  // First is valid when there is data to add.
  //
  if (first != std::string::npos && first < data.length()) {
    if (_buffer.empty() && data[first] != '$') {
      _delegate->onInvalidData(data.substr(first));
      return;
    }
    _buffer += data.substr(first);
  }
}
} // namespace GDBRemote
} // namespace ds2
