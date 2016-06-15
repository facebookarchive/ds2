//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_GDBRemote_ProtocolHelpers_h
#define __DebugServer2_GDBRemote_ProtocolHelpers_h

#include "DebugServer2/Types.h"

namespace ds2 {
namespace GDBRemote {

uint8_t Checksum(std::string const &data);

std::string Escape(std::string const &data);

std::string Unescape(std::string const &data);
}
}

#endif // !__DebugServer2_GDBRemote_ProtocolHelpers_h
