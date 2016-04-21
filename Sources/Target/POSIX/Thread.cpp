//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/POSIX/Thread.h"
#include "DebugServer2/Target/Process.h"

#include <sys/wait.h>

#define super ds2::Target::ThreadBase

namespace ds2 {
namespace Target {
namespace POSIX {

Thread::Thread(ds2::Target::Process *process, ThreadId tid)
    : super(process, tid) {}

ErrorCode Thread::updateStopInfo(int waitStatus) {
  _stopInfo.clear();

  if (WIFEXITED(waitStatus)) {
    _stopInfo.event = StopInfo::kEventExit;
    _stopInfo.status = WEXITSTATUS(waitStatus);
  } else if (WIFSIGNALED(waitStatus)) {
    _stopInfo.event = StopInfo::kEventKill;
    _stopInfo.signal = WTERMSIG(waitStatus);
  } else if (WIFSTOPPED(waitStatus)) {
    _stopInfo.event = StopInfo::kEventStop;
    _stopInfo.signal = WSTOPSIG(waitStatus);
  } else {
    // On POSIX systems, the status returned by `waitpid()` references either a
    // process that exited (WIFEXITED), was killed with a signal (WIFSIGNALED),
    // or was stopped (WIFSTOPPED). On Linux since 2.6.10, we can also check
    // for processes being continued (WIFCONTINUED), but we don't use that (no
    // mention of WCONTINUED in wait flags).
    DS2BUG("impossible waitStatus");
  }

  return kSuccess;
}
}
}
}
