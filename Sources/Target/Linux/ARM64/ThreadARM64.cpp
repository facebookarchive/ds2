//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#define __DS2_LOG_CLASS_NAME__ "Target::Thread"

#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Architecture/ARM/Branching.h"
#include "DebugServer2/BreakpointManager.h"
#include "DebugServer2/Utils/Log.h"

using ds2::Target::Linux::Process;

namespace ds2 {
namespace Target {
namespace Linux {

ErrorCode Thread::prepareSoftwareSingleStep(Address const &address) {
  return kSuccess;
}
}
}
}
