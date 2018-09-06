//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/GDBRemote/SessionBase.h"
#include "DebugServer2/GDBRemote/ProtocolHelpers.h"
#include "DebugServer2/Utils/HexValues.h"
#include "DebugServer2/Utils/Log.h"

#include <iomanip>
#include <sstream>

namespace ds2 {
namespace GDBRemote {

SessionBase::SessionBase(CompatibilityMode mode)
    : _channel(nullptr), _delegate(nullptr), _ackmode(true), _compatMode(mode) {
  _processor.setDelegate(&_interpreter);
  _interpreter.setSession(this);
}

bool SessionBase::create(Host::Channel *channel) {
  if (_channel != nullptr || channel == nullptr)
    return false;

  _channel = channel;
  return true;
}

bool SessionBase::receive(bool cooked) {
  if (_channel == nullptr)
    return false;

  if (!_channel->wait())
    return false;

  std::string data;

  if (!_channel->receive(data))
    return false;

  if (data.empty())
    return true;

  if (cooked) {
    //
    // If data is 'cooked', then it has been already processed
    // by the Packet Processor, so we just need to forward it
    // to the interpreter.
    //
    _interpreter.onPacketData(data, true);
    return true;
  }

  return parse(data);
}

bool SessionBase::parse(std::string const &data) {
  if (data.empty())
    return false;

  _processor.parse(data);
  return true;
}

//
// Functions used by the ProtocolInterpreter
//
bool SessionBase::onACK() {
  // TODO implement me
  return true;
}

bool SessionBase::onNAK() {
  // TODO implement me
  return true;
}

bool SessionBase::onCommandReceived(bool valid) {
  //
  // Send ACK or NAK if in acknowledge mode.
  //
  if (!_ackmode)
    return true;

  return valid ? sendACK() : sendNAK();
}

void SessionBase::onInvalidData(std::string const &) {
  //
  // Send NAK in acknowledge mode.
  //
  sendNAK();
}

//
// Send commands
//
bool SessionBase::sendACK() { return _channel->send("+", 1) == 1; }

bool SessionBase::sendNAK() { return _channel->send("-", 1) == 1; }

// The GDB protocol specifies whitespace in some packets. However,
// lldb-server does not use this whitespace, and older versions of
// lldb will fail if it is used. Don't use a separator in lldb mode.
const char *SessionBase::getPacketSeparator() {
  switch (_compatMode) {
  case kCompatibilityModeGDB:
  case kCompatibilityModeGDBMultiprocess:
    return " ";

  case kCompatibilityModeLLDB:
    return "";

  case kCompatibilityModeLLDBThread:
    DS2BUG("LLDBThreads is an invalid compatibility mode for SessionBase");
  }

  DS2_UNREACHABLE();
}

bool SessionBase::sendError(ErrorCode code) {
  switch (code) {
  case kSuccess:
    return sendOK();

  case kErrorUnsupported:
  case kErrorUnknown:
    return send("");

  default:
    break;
  }

  std::ostringstream ss;
  ss << "E" << getPacketSeparator();
  ss << NibbleToHex(code >> 4) << NibbleToHex(code & 15);

  return send(ss.str());
}
} // namespace GDBRemote
} // namespace ds2
