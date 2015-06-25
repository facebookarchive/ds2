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
#include "DebugServer2/Target/Linux/Thread.h"
#include "DebugServer2/Host/Linux/PTrace.h"
#include "DebugServer2/Host/Linux/ProcFS.h"
#include "DebugServer2/Utils/Log.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

#define super ds2::Target::POSIX::Thread

using ds2::Host::Linux::ProcFS;

namespace ds2 {
namespace Target {
namespace Linux {

Thread::Thread(Process *process, ThreadId tid) : super(process, tid) {
  //
  // Initially the thread is stopped.
  //
  _state = kStopped;
}

Thread::~Thread() {}

ErrorCode Thread::terminate() {
  return process()->ptrace().kill(ProcessThreadId(process()->pid(), tid()),
                                  SIGKILL);
}

ErrorCode Thread::suspend() {
  ErrorCode error = kSuccess;
  if (_state == kRunning) {
    error =
        process()->ptrace().suspend(ProcessThreadId(process()->pid(), tid()));
    if (error != kSuccess)
      return error;

    int status;
    error = process()->ptrace().wait(ProcessThreadId(process()->pid(), tid()),
                                     true, &status);
    if (error != kSuccess) {
      DS2LOG(Error, "failed to wait for tid %d, error=%s\n", tid(),
             strerror(errno));
      return error;
    }

    updateStopInfo(status);
  }

  if (_state == kTerminated) {
    error = kErrorProcessNotFound;
  }

  return error;
}

ErrorCode Thread::step(int signal, Address const &address) {
  ErrorCode error = kSuccess;
  if (_state == kStopped || _state == kStepped) {
    DS2LOG(Debug, "stepping tid %d", tid());
    if (process()->isSingleStepSupported()) {
      ProcessInfo info;

      error = process()->getInfo(info);
      if (error != kSuccess)
        return error;

      error = process()->ptrace().step(ProcessThreadId(process()->pid(), tid()),
                                       info, signal, address);

      if (error == kSuccess) {
        _state = kStepped;
      }
    } else {
      //
      // Prepare a software (arch-dependent) single step and
      // resume execution.
      //
      error = prepareSoftwareSingleStep(address);
      if (error != kSuccess)
        return error;

      error = resume(signal, address);
    }
  } else if (_state == kTerminated) {
    error = kErrorProcessNotFound;
  }
  return error;
}

ErrorCode Thread::resume(int signal, Address const &address) {
  ErrorCode error = kSuccess;

  if (_state == kStopped || _state == kStepped) {
    if (signal == 0) {
      switch (_trap.signal) {
      case SIGCHLD:
      case SIGSTOP:
      case SIGTRAP:
        signal = 0;
        break;
      default:
        signal = _trap.signal;
        break;
      }
    }

    ProcessInfo info;

    error = process()->getInfo(info);
    if (error != kSuccess)
      return error;

    error = process()->ptrace().resume(ProcessThreadId(process()->pid(), tid()),
                                       info, signal, address);
    if (error == kSuccess) {
      _state = kRunning;
      _trap.signal = 0;
    }
  } else if (_state == kTerminated) {
    error = kErrorProcessNotFound;
  }

  return error;
}

ErrorCode Thread::readCPUState(Architecture::CPUState &state) {
  // TODO cache CPU state
  ProcessInfo info;
  ErrorCode error;

  error = _process->getInfo(info);
  if (error != kSuccess)
    return error;

  return process()->ptrace().readCPUState(
      ProcessThreadId(process()->pid(), tid()), info, state);
}

ErrorCode Thread::writeCPUState(Architecture::CPUState const &state) {
  ProcessInfo info;
  ErrorCode error;

  error = _process->getInfo(info);
  if (error != kSuccess)
    return error;

  return process()->ptrace().writeCPUState(
      ProcessThreadId(process()->pid(), tid()), info, state);
}

ErrorCode Thread::updateStopInfo(int waitStatus) {
  super::updateStopInfo(waitStatus);

  updateState();

  switch (_trap.event) {
  case StopInfo::kEventNone:
    DS2BUG("thread stopped for unknown reason, status=%#x", waitStatus);

  case StopInfo::kEventExit:
  case StopInfo::kEventKill:
    DS2ASSERT(_trap.reason == StopInfo::kReasonNone);
    return kSuccess;

  case StopInfo::kEventStop: {
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
    // (2) we sent the thread a SIGSTOP (with tkill) to interrupt it e.g.:
    //     a thread hits a breakpoint, we have to stop every other thread.
    //     These other treads will be mark as no reason so the debugger can
    //     adapt its output (e.g.: lldb will simply hide these threads and only
    //     display the one that stopped for a breakpoint);
    // (3) the inferior received a SIGSTOP because of ptrace attach. We have to
    //     mark the thread as stopped for a trap;
    // (4) the inferior received a SIGTRAP. This is usually because of a
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
      _trap.event = StopInfo::kEventNone;
    } else if (si.si_code == SI_TKILL && si.si_pid == getpid()) { // (2)
      // The only signal we are supposed to send to the inferior is a
      // SIGSTOP anyway.
      DS2ASSERT(_trap.signal == SIGSTOP);
      _trap.event = StopInfo::kEventNone;
    } else if (si.si_code == SI_USER && si.si_pid == 0 &&
               _trap.signal == SIGSTOP) { // (3)
      _trap.reason = StopInfo::kReasonTrap;
    } else if (_trap.signal == SIGTRAP) { // (4)
      _trap.reason = StopInfo::kReasonBreakpoint;
    } else {
      // This is not a signal that we originated. We can output a
      // warning if the signal comes from an external source.
      _trap.reason = StopInfo::kReasonSignalStop;
      if ((si.si_code == SI_USER || si.si_code == SI_TKILL) &&
          si.si_pid != tid())
        DS2LOG(Warning,
               "tid %d received signal %s from an external source (sender=%d)",
               tid(), strsignal(_trap.signal), si.si_pid);
    }
  } break;
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

  _trap.core = stat.task_cpu;

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
}
}
}
