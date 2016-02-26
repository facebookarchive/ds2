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

#define __DS2_LOG_CLASS_NAME__ "Target::Thread"

#include "DebugServer2/Target/Darwin/Thread.h"
#include "DebugServer2/Architecture/CPUState.h"
#include "DebugServer2/Host/Darwin/LibProc.h"
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

using Host::Darwin::LibProc;

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

ErrorCode Thread::updateStopInfo(Host::Darwin::MachExcStatus &status) {

  switch (status.type) {
  case EXC_BREAKPOINT:
    _stopInfo.event = StopInfo::kEventStop;
    _stopInfo.reason = StopInfo::kReasonBreakpoint;
    _stopInfo.signal = SIGTRAP; // Simulate other POSIX system
    break;
  case EXC_SOFTWARE:
    _stopInfo.event = StopInfo::kEventStop;
    if (status.subtype == EXC_SOFT_SIGNAL) {
      _stopInfo.signal = status.data;
      if (status.data == SIGTRAP) {
        _stopInfo.reason = StopInfo::kReasonBreakpoint;
      }
      break;
    }
  default:
    _stopInfo.event = StopInfo::kEventNone;
    _stopInfo.reason = StopInfo::kReasonNone;
  }

  updateState();
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
}
}
}
