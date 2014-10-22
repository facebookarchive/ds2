//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Target/Process.h"

using ds2::Target::Linux::Thread;
using ds2::Target::Linux::Process;
using ds2::ErrorCode;

ErrorCode Thread::prepareSoftwareSingleStep(Address const &address) {
  return kSuccess;
}
