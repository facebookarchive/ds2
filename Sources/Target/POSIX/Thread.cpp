//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Target/POSIX/Thread.h"

#include <sys/wait.h>

#define super ds2::Target::ThreadBase

namespace ds2 {
namespace Target {
namespace POSIX {

Thread::Thread(ds2::Target::Process *process, ThreadId tid)
    : super(process, tid) {}

ErrorCode Thread::updateStopInfo(int waitStatus) {
  ErrorCode error = kSuccess;

  _trap.clear();

  if (WIFEXITED(waitStatus)) {
    _trap.event = StopInfo::kEventExit;
    _trap.status = WEXITSTATUS(waitStatus);
  } else if (WIFSIGNALED(waitStatus)) {
    _trap.event = StopInfo::kEventKill;
    _trap.status = WEXITSTATUS(waitStatus);
    _trap.signal = WTERMSIG(waitStatus);
  } else if (WIFSTOPPED(waitStatus)) {
    _trap.event = StopInfo::kEventStop;
    _trap.signal = WSTOPSIG(waitStatus);
  }

  return error;
}
}
}
}
