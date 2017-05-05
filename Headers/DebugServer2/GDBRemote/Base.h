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

#include "DebugServer2/Types.h"

namespace ds2 {
namespace GDBRemote {

enum CompatibilityMode {
  kCompatibilityModeGDB,
  kCompatibilityModeGDBMultiprocess,
  kCompatibilityModeLLDB,
  // This is a special case for GDBRemote::ProcessThreadId::encode,
  // don't use it anywhere else!
  kCompatibilityModeLLDBThread
};

enum AttachMode { kAttachNow, kAttachAndWait, kAttachOrWait };

enum BreakpointType : unsigned {
  kSoftwareBreakpoint = 0,
  kHardwareBreakpoint = 1,
  kWriteWatchpoint = 2,
  kReadWatchpoint = 3,
  kAccessWatchpoint = 4,
  kBreakpointTypeMax = 5
};
} // namespace GDBRemote
} // namespace ds2
