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

#include "DebugServer2/GDBRemote/PacketProcessor.h"
#include "DebugServer2/GDBRemote/ProtocolHelpers.h"
#include "DebugServer2/GDBRemote/ProtocolInterpreter.h"
#include "DebugServer2/GDBRemote/Types.h"
#include "DebugServer2/Host/Channel.h"
#include "DebugServer2/Utils/Log.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <type_traits>

namespace ds2 {
namespace GDBRemote {

class SessionDelegate;

class SessionBase : public ProtocolHandler {
private:
  Host::Channel *_channel;
  PacketProcessor _processor;
  ProtocolInterpreter _interpreter;

protected:
  SessionDelegate *_delegate;
  bool _ackmode;
  CompatibilityMode _compatMode;

public:
  SessionBase(CompatibilityMode mode);
  virtual ~SessionBase() = default;

public:
  CompatibilityMode mode() const { return _compatMode; }

protected:
  const char *getPacketSeparator();

public:
  inline void setDelegate(SessionDelegate *delegate) { _delegate = delegate; }
  inline SessionDelegate *delegate() const {
    return const_cast<SessionBase *>(this)->_delegate;
  }

public:
  bool create(Host::Channel *channel);

public:
  bool receive(bool cooked);
  bool parse(std::string const &data);

public:
  bool send(char const *data, bool escaped = false) {
    return send(std::string(data), escaped);
  }

  template <typename T> bool send(T const &data, bool escaped = false) {
    std::ostringstream ss;
    static std::string const searchStr = "$#}*";
    uint8_t csum;

    ss << '$';

    //
    // If data contains $, #, } or * we need to escape the
    // stream.
    //
    if (!escaped &&
        std::find_first_of(data.begin(), data.end(), searchStr.begin(),
                           searchStr.end()) != data.end()) {
      std::string encoded = Escape(data);
      ss << encoded;
      csum = Checksum(encoded);
    } else {
      for (char c : data) {
        ss << c;
      }
      csum = Checksum(data);
    }

    ss << '#' << std::hex << std::setw(2) << std::setfill('0')
       << (unsigned)csum;

    std::string final_data = ss.str();
    DS2LOG(Packet, "putpkt(\"%s\", %u)", final_data.c_str(),
           (unsigned)final_data.length());

    return _channel->send(final_data);
  }

protected:
  bool sendACK();
  bool sendNAK();
  inline bool sendOK() { return send("OK"); }
  bool sendError(ErrorCode code);

public:
  inline bool getAckMode() { return _ackmode; }

protected:
  inline void setAckMode(bool enabled) { _ackmode = enabled; }

public:
  inline ProtocolInterpreter &interpreter() const {
    return const_cast<SessionBase *>(this)->_interpreter;
  }

protected:
  friend class ProtocolInterpreter;
  virtual bool onACK();
  virtual bool onNAK();
  virtual bool onCommandReceived(bool valid);
  virtual void onInvalidData(std::string const &data);
};
} // namespace GDBRemote
} // namespace ds2
