//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#define __DS2_LOG_CLASS_NAME__ "Session"

#include "DebugServer2/GDBRemote/SessionBase.h"
#include "DebugServer2/GDBRemote/SessionDelegate.h"
#include "DebugServer2/GDBRemote/ProtocolHelpers.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/HexValues.h"
#include "DebugServer2/Utils/Log.h"

#include <iomanip>
#include <sstream>

using ds2::Host::Platform;

namespace ds2 {
namespace GDBRemote {

SessionBase::SessionBase()
    : _channel(nullptr), _delegate(nullptr), _ackmode(true) {
  _processor.setDelegate(&_interpreter);
  _interpreter.setSession(this);
}

SessionBase::~SessionBase() { delete _channel; }

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

bool SessionBase::send(std::string const &data, bool escaped) {
  std::ostringstream ss;
  std::string encoded;
  std::string const *datap = &data;

  //
  // If data contains $, #, } or * we need to escape the
  // stream.
  //
  if (!escaped && data.find_first_of("$#}*") != std::string::npos) {
    encoded = Escape(data);
    datap = &encoded;
  }

  uint8_t csum = Checksum(*datap);

  ss << '$' << *datap << '#' << std::hex << std::setw(2) << std::setfill('0')
     << (unsigned)csum;

  std::string final_data = ss.str();
  DS2LOG(Remote, Debug, "putpkt(\"%s\", %u)", final_data.c_str(),
         (unsigned)final_data.length());

  return _channel->send(final_data);
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

  char message[4];
  message[0] = 'E';
  message[1] = NibbleToHex(code >> 4);
  message[2] = NibbleToHex(code & 15);
  message[3] = '\0';
  return send(message);
}

using ds2::GDBRemote::SessionDelegate;

SessionDelegate::~SessionDelegate() {}
}
}
