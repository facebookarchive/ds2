//
// Copyright (c) 2014-present, Facebook, Inc.
// Copyright (c) 2015, Jakub Klama <jakub@ixsystems.com>
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/Darwin/Thread.h"
#include "DebugServer2/Architecture/CPUState.h"
#include "DebugServer2/Host/Darwin/PTrace.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Utils/Log.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/syscall.h>
#include <sys/wait.h>

#define super ds2::Target::POSIX::Thread

namespace ds2 {
namespace Target {
namespace Darwin {

Thread::Thread(ds2::Target::Process *process, ThreadId tid)
    : super(process, tid), _lastSyscallNumber(-1) {}

ErrorCode Thread::readCPUState(Architecture::CPUState &state) {
  // TODO cache CPU state
  ProcessInfo info;
  ErrorCode error = _process->getInfo(info);
  if (error != kSuccess)
    return error;

  return process()->mach().readCPUState(
      ProcessThreadId(process()->pid(), tid()), info, state);
}

ErrorCode Thread::writeCPUState(Architecture::CPUState const &state) {
  ProcessInfo info;
  ErrorCode error = _process->getInfo(info);
  if (error != kSuccess)
    return error;

  return process()->mach().writeCPUState(
      ProcessThreadId(process()->pid(), tid()), info, state);
}

ErrorCode Thread::updateStopInfo(int waitStatus) {
  super::updateStopInfo(waitStatus);
  updateState();

  switch (_stopInfo.event) {
  case StopInfo::kEventNone:
    DS2BUG("thread stopped for unknown reason, status=%#x", waitStatus);

  case StopInfo::kEventExit:
  case StopInfo::kEventKill:
    DS2ASSERT(_stopInfo.reason == StopInfo::kReasonNone);
    return kSuccess;

  case StopInfo::kEventStop: {
    // TODO: Here I need to have a real management of signal, but
    // for the moment, likely when you strat, it will be a breakpoint
    _stopInfo.reason = StopInfo::kReasonBreakpoint;

  } break;
  }

  return kSuccess;
}

void Thread::updateState() { updateState(false); }

void Thread::updateState(bool force) {
  if (!process()->isAlive()) {
    _state = kTerminated;
    return;
  }

  struct thread_basic_info info;
  ProcessThreadId ptid(process()->pid(), tid());
  ErrorCode error = process()->mach().getThreadInfo(ptid, &info);
  if (error != kSuccess) {
    info.run_state = 0;
  }

  switch (info.run_state) {
  case TH_STATE_HALTED:
    _state = kTerminated;
    break;

  case TH_STATE_RUNNING:
  case TH_STATE_UNINTERRUPTIBLE:
    _state = kRunning;
    break;

  case TH_STATE_WAITING:
  case TH_STATE_STOPPED:
    _state = kStopped;
    break;

  default:
    _state = kInvalid;
    break;
  }

  _stopInfo.core = 0; // TODO
}
} // namespace Darwin
} // namespace Target
} // namespace ds2
