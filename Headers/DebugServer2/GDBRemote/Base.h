//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_GDBRemote_Base_h
#define __DebugServer2_GDBRemote_Base_h

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

enum BreakpointType {
  kSoftwareBreakpoint = 0,
  kHardwareBreakpoint = 1,
  kWriteWatchpoint = 2,
  kReadWatchpoint = 3,
  kAccessWatchpoint = 4
};
}
}

#endif // !__DebugServer2_GDBRemote_Base_h
