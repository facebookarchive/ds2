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
#include "DebugServer2/Utils/Log.h"

namespace ds2 {
namespace Target {
namespace Windows {

ErrorCode Thread::writeCPUState(Architecture::CPUState const &state) {
  DS2BUG("not implemented");
}

ErrorCode Thread::readCPUState(Architecture::CPUState &state) {
  DS2BUG("not implemented");
}

ErrorCode Thread::step(int signal, Address const &address) {
  DS2BUG("not implemented");
}
}
}
}
