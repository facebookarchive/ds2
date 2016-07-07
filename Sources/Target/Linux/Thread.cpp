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

#include "DebugServer2/Target/Linux/Thread.h"
#include "DebugServer2/Host/Linux/ExtraWrappers.h"
#include "DebugServer2/Host/Linux/PTrace.h"
#include "DebugServer2/Host/Linux/ProcFS.h"
#include "DebugServer2/SoftwareBreakpointManager.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Utils/Log.h"
#include "DebugServer2/Utils/Stringify.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

using ds2::Host::Linux::ProcFS;
using ds2::Utils::Stringify;

#define super ds2::Target::POSIX::Thread

namespace ds2 {
namespace Target {
namespace Linux {

Thread::Thread(Process *process, ThreadId tid) : super(process, tid) {}

void Thread::fillWatchpointData() {
  HardwareBreakpointManager *hwBpm = process()->hardwareBreakpointManager();

  if (hwBpm) {
    BreakpointManager::Site site;

    if (hwBpm->hit(this, site) >= 0) {
      switch (static_cast<int>(site.mode)) {
      case BreakpointManager::kModeExec:
        _stopInfo.reason = StopInfo::kReasonBreakpoint;
        return;
      case BreakpointManager::kModeWrite:
        _stopInfo.reason = StopInfo::kReasonWriteWatchpoint;
        return;
      case BreakpointManager::kModeRead:
        _stopInfo.reason = StopInfo::kReasonReadWatchpoint;
        return;
      case BreakpointManager::kModeRead | BreakpointManager::kModeWrite:
        _stopInfo.reason = StopInfo::kReasonAccessWatchpoint;
        return;
      default:
        DS2BUG("invalid mode");
      }
    }
  }

  _stopInfo.reason = StopInfo::kReasonTrace;
}

ErrorCode Thread::updateStopInfo(int waitStatus) {
  super::updateStopInfo(waitStatus);

  switch (_stopInfo.event) {
  case StopInfo::kEventExit:
  case StopInfo::kEventKill:
    DS2ASSERT(_stopInfo.reason == StopInfo::kReasonNone);
    _state = kTerminated;
    return kSuccess;

  case StopInfo::kEventStop: {
    _state = kStopped;

    // These are the reasons why we might want to alter the stop info of a
    // thread:
    // (1) a thread traced with PTRACE_O_TRACECLONE calls clone(2), it (the
    //     caller of clone(2)) is stopped with a SIGTRAP, and control is given
    //     back to the tracer (us). The wait(2) status will then be constructed
    //     so that
    //       status >> 8 == (SIGTRAP | (PTRACE_EVENT_CLONE << 8))
    //     which results in WIFSTOPPED(status) == true and
    //     WSTOPSIG(status) == SIGTRAP. We mark the thread stopped for no
    //     reason so it just gets restarted immediately (see
    //     Linux::Process::wait);
    // (2) we sent the thread a SIGSTOP (with tkill(2)) to suspend it e.g.:
    //     when a thread hits a breakpoint, we have to stop every other thread,
    //     so we send each one of them a SIGSTOP with tkill(2). These other
    //     treads will be marked as stopped for no reason so the debugger can
    //     adapt its output (e.g.: lldb will simply hide these threads and only
    //     display the one that stopped for a breakpoint);
    // (3) we sent the process a SIGSTOP (with kill(2)) to interrupt it
    //     entirely. This happens when the user hits Ctrl-C and the debugger
    //     sends us a "\x03" for instance;
    // (4) the inferior received a SIGSTOP because of ptrace attach. We have to
    //     mark the thread as stopped for a trap;
    // (5) the inferior received a SIGTRAP. This is usually because of a
    //     breakpoint, single step or such;

    siginfo_t si;
    ProcessThreadId ptid(process()->pid(), tid());
    ErrorCode error = process()->ptrace().getSigInfo(ptid, si);
    if (error != kSuccess) {
      DS2LOG(Warning, "unable to get siginfo_t for tid %d, error=%s", tid(),
             strerror(errno));
      return error;
    }

    if (waitStatus >> 8 == (SIGTRAP | (PTRACE_EVENT_CLONE << 8))) { // (1)
      _stopInfo.event = StopInfo::kEventNone;
    } else if (si.si_code == SI_TKILL && si.si_pid == getpid()) { // (2)
      // The only signal we are supposed to send to the inferior is a SIGSTOP.
      DS2ASSERT(_stopInfo.signal == SIGSTOP);
      _stopInfo.event = StopInfo::kEventNone;
    } else if (si.si_code == SI_USER && si.si_pid == getpid()) { // (3)
      DS2ASSERT(_stopInfo.signal == SIGSTOP);
      _stopInfo.reason = StopInfo::kReasonSignalStop;
    } else if (si.si_code == SI_USER && si.si_pid == 0 &&
               _stopInfo.signal == SIGSTOP) { // (4)
      _stopInfo.reason = StopInfo::kReasonTrap;
    } else if (_stopInfo.signal == SIGTRAP) { // (5)
      switch (si.si_code) {
      case 0:
        _stopInfo.reason = StopInfo::kReasonTrap;
        break;
      case TRAP_HWBKPT:
      case TRAP_TRACE:
        fillWatchpointData();
        break;
      case SI_KERNEL:
      case TRAP_BRKPT:
        _stopInfo.reason = StopInfo::kReasonBreakpoint;
        break;
      default:
        DS2BUG("unknown sigtrap code");
      }
    } else {
      // This is not a signal that we originated. We can output a
      // warning if the signal comes from an external source.
      _stopInfo.reason = StopInfo::kReasonSignalStop;
      if ((si.si_code == SI_USER || si.si_code == SI_TKILL) &&
          si.si_pid != tid())
        DS2LOG(Warning,
               "tid %d received signal %s from an external source (sender=%d)",
               tid(), strsignal(_stopInfo.signal), si.si_pid);
    }
  } break;

  default:
    DS2BUG("impossible StopInfo event: %s",
           Stringify::StopEvent(_stopInfo.event));
  }

  return kSuccess;
}

void Thread::updateState() {
  if (!process()->isAlive()) {
    _state = kTerminated;
    return;
  }

  ProcFS::Stat stat;
  if (!ProcFS::ReadStat(_process->pid(), tid(), stat)) {
    stat.task_cpu = 0;
    stat.state = 0;
  }

  _stopInfo.core = stat.task_cpu;

  switch (stat.state) {
  case Host::Linux::kProcStateZombie:
  case Host::Linux::kProcStateDead:
    _state = kTerminated;
    break;

  case Host::Linux::kProcStateUninterruptible:
  case Host::Linux::kProcStateSleeping:
  case Host::Linux::kProcStateRunning:
  case Host::Linux::kProcStatePaging:
    _state = kRunning;
    break;

  case Host::Linux::kProcStateTraced:
  case Host::Linux::kProcStateStopped:
    _state = kStopped;
    break;

  default:
    _state = kInvalid;
    break;
  }
}

#if defined(ARCH_X86) || defined(ARCH_X86_64)
uintptr_t Thread::readDebugReg(size_t idx) const {
  return process()->ptrace().readDebugReg(tid(), idx);
}

ErrorCode Thread::writeDebugReg(size_t idx, uintptr_t val) const {
  return process()->ptrace().writeDebugReg(tid(), idx, val);
}
#endif
}
}
}
