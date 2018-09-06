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

#include "DebugServer2/Target/FreeBSD/Thread.h"
#include "DebugServer2/Architecture/CPUState.h"
#include "DebugServer2/Host/FreeBSD/PTrace.h"
#include "DebugServer2/Host/FreeBSD/ProcStat.h"
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
namespace FreeBSD {

using Host::FreeBSD::ProcStat;

Thread::Thread(Process *process, ThreadId tid)
    : super(process, tid), _lastSyscallNumber(-1) {}

ErrorCode Thread::updateStopInfo(int waitStatus) {
  super::updateStopInfo(waitStatus);
  struct ptrace_lwpinfo lwpinfo;
  updateState();

  switch (_stopInfo.event) {
  case StopInfo::kEventNone:
    DS2BUG("thread stopped for unknown reason, status=%#x", waitStatus);

  case StopInfo::kEventExit:
  case StopInfo::kEventKill:
    DS2ASSERT(_stopInfo.reason == StopInfo::kReasonNone);
    return kSuccess;

  case StopInfo::kEventStop: {
    siginfo_t *si;
    ErrorCode error = process()->_ptrace.getLwpInfo(process()->pid(), &lwpinfo);
    if (error != kSuccess) {
      DS2LOG(Warning, "unable to get siginfo_t for tid %d, error=%s", tid(),
             strerror(errno));
      return error;
    }

    si = &lwpinfo.pl_siginfo;
    if (si->si_code == SI_USER && si->si_pid == 0 &&
        _stopInfo.signal == SIGSTOP) { // (3)
      _stopInfo.reason = StopInfo::kReasonTrap;
    } else if (_stopInfo.signal == SIGTRAP) { // (4)
      _stopInfo.reason = StopInfo::kReasonBreakpoint;
    } else {
      // This is not a signal that we originated. We can output a
      // warning if the signal comes from an external source.
      DS2LOG(Warning,
             "tid %d received signal %s from an external source (sender=%d)",
             tid(), strsignal(_stopInfo.signal), si->si_pid);
    }
  } break;
  }

  return kSuccess;
}

void Thread::updateState() { updateState(false); }

void Thread::updateState(bool force) {
  int state = 0;
  int cpu = 0;
  if (!process()->isAlive()) {
    _state = kTerminated;
    return;
  }

  ProcStat::GetThreadState(_process->pid(), tid(), state, cpu);
  _stopInfo.core = cpu;

  switch (state) {
  case Host::FreeBSD::kProcStateDead:
  case Host::FreeBSD::kProcStateZombie:
    _state = kTerminated;
    break;

  case Host::FreeBSD::kProcStateSleeping:
  case Host::FreeBSD::kProcStateRunning:
  case Host::FreeBSD::kProcStateWaiting:
  case Host::FreeBSD::kProcStateLock:
    _state = kRunning;
    break;

  case Host::FreeBSD::kProcStateStopped:
    _state = kStopped;
    break;

  default:
    _state = kInvalid;
    break;
  }
}
} // namespace FreeBSD
} // namespace Target
} // namespace ds2
