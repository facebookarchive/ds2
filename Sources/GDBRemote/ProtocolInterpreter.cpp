//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/GDBRemote/ProtocolInterpreter.h"
#include "DebugServer2/GDBRemote/ProtocolHelpers.h"
#include "DebugServer2/GDBRemote/Session.h"
#include "DebugServer2/Utils/Log.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace ds2 {
namespace GDBRemote {

static std::string EscapeForTerm(std::string const &s) {
  std::ostringstream ss;
  for (char n : s) {
    unsigned c = static_cast<unsigned>(n & 0xff);
    if (c < 0x20 || c > 0x7f) {
      ss << "\\x" << std::hex << std::setw(2) << std::setfill('0') << c
         << std::dec;
    } else {
      ss << n;
    }
  }
  return ss.str();
}

ProtocolInterpreter::ProtocolInterpreter() : _session(nullptr) {}

void ProtocolInterpreter::onPacketData(std::string const &data, bool valid) {
  DS2LOG(Packet, "getpkt(\"%s\")", EscapeForTerm(&data[0]).c_str());

  if (_session == nullptr)
    return;

  if (data.length() == 1) {
    //
    // ACK and NAKs are handled specially.
    //
    switch (data[0]) {
    case '+':
      _session->onACK();
      return;

    case '-':
      _session->onNAK();
      return;

    default:
      break;
    }
  }

  //
  // Inform the session that we received a command, if it's not
  // valid, the session may resend the previous command or send
  // an ack or a nak.
  //
  if (!_session->onCommandReceived(valid) || !valid)
    return;

  //
  // Extract the command and arguments to pass down to the
  // handler.
  //
  size_t command_end = std::string::npos;
  size_t args_start = std::string::npos;
  if (data[0] == 'v' || data[0] == 'q' || data[0] == 'Q') {
    //
    // The commands starting with 'v', 'q' or 'Q' may be terminated
    // by one of the following separator: , (comma), : (colon) or
    // ; (semi-colon).
    //
    size_t end = data.find_first_of(",:;");
    if (end != std::string::npos) {
      command_end = end;
      args_start = end + 1;
    }
  } else if (data[0] == 'b') {
    //
    // The commands starting with 'b' may be two chars long; only 'bc'
    // and 'bs' are known.
    //
    if (data.length() == 2 && (data[1] == 'c' || data[1] == 's')) {
      command_end = 2;
    } else {
      command_end = 1;
    }
  } else if (data[0] == '_') {
    //
    // The commands starting with '_' may be two chars long; only '_M'
    // and '_m' are known.
    //
    if (data.length() > 1 && (data[1] == 'M' || data[1] == 'm')) {
      command_end = 2;
    } else {
      command_end = 1;
    }
  } else if (data[0] == 'j') {
    //
    // The commands starting with j are terminated with : (colon)
    //
    size_t end = data.find_first_of(":");
    if (end != std::string::npos) {
      command_end = end;
      args_start = end + 1;
    }
  } else {
    //
    // Any other command is long just one char.
    //
    command_end = 1;
  }

  if (args_start == std::string::npos && command_end < data.length()) {
    //
    // Arguments follow the command with no separator.
    //
    args_start = command_end;
  }

  std::string command = data.substr(0, command_end);
  std::string args;
  if (args_start != std::string::npos) {
    args = data.substr(args_start);
  }

  //
  // Find the handler and execute it.
  //
  onCommand(command, args);
}

void ProtocolInterpreter::onInvalidData(std::string const &data) {
  DS2LOG(Warning, "received invalid data: '%s'", data.c_str());

  if (_session == nullptr)
    return;

  _session->onInvalidData(data);
}

void ProtocolInterpreter::onCommand(std::string const &command,
                                    std::string const &arguments) {
  size_t commandLength;
  Handler const *handler = findHandler(command, commandLength);
  if (handler == nullptr) {
    DS2LOG(Packet, "handler for command '%s' unknown", command.c_str());

    //
    // The handler couldn't be found, we don't support this packet.
    //
    _session->sendError(kErrorUnsupported);
    return;
  }

  std::string extra;
  if (commandLength != command.length()) {
    //
    // Command has part of the argument, LLDB doesn't use separators :(
    //
    extra = command.substr(commandLength);
  }

  extra += arguments;

  if (extra.find_first_of("*}") != std::string::npos) {
    extra = Unescape(extra);
    DS2LOG(Packet, "args='%.*s'", static_cast<int>(extra.length()), &extra[0]);
  }

  (handler->handler->*handler->callback)(*handler, extra);
}

bool ProtocolInterpreter::registerHandler(Handler const &handler) {
  if (handler.command.empty() || handler.handler == nullptr ||
      handler.callback == nullptr)
    return false;

  size_t commandLength;
  if (findHandler(handler.command, commandLength))
    return false;

  _handlers.push_back(handler);

  // We need to sort the handlers vector.
  std::sort(_handlers.begin(), _handlers.end(),
            [](Handler const &a, Handler const &b) -> bool {
              return (a.command < b.command);
            });

  return true;
}

ProtocolInterpreter::Handler const *
ProtocolInterpreter::findHandler(std::string const &command,
                                 size_t &commandLength) const {
  auto it = std::lower_bound(
      _handlers.begin(), _handlers.end(), command,
      [](Handler const &handler, std::string const &command) -> bool {
        return handler.compare(command) < 0;
      });

  Handler const *handler = nullptr;
  if (it != _handlers.end() && it->compare(command) == 0) {
    commandLength = it->command.length();
    handler = &(*it);
  }

  return handler;
}

int ProtocolInterpreter::Handler::compare(std::string const &command_) const {
  if (mode == Handler::kModeEquals) {
    return command.compare(command_);
  } else {
    return command.compare(command_.substr(0, command.length()));
  }
}
} // namespace GDBRemote
} // namespace ds2
