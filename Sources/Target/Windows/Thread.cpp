//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#define __DS2_LOG_CLASS_NAME__ "Target::Thread"

#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Target/Windows/Thread.h"
#include "DebugServer2/Utils/Log.h"

#define super ds2::Target::ThreadBase

namespace ds2 {
namespace Target {
namespace Windows {

Thread::Thread(Process *process, ThreadId tid, HANDLE handle)
    : super(process, tid), _handle(handle) {
  //
  // Initially the thread is stopped.
  //
  _state = kStopped;
}

Thread::~Thread() { CloseHandle(_handle); }
}
}
}
