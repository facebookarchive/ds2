//
// Copyright (c) 2014, Facebook, Inc.
// Copyright (c) 2015, Jakub Klama <jakub@ixsystems.com>
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#define __DS2_LOG_CLASS_NAME__ "Target::Thread"

#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Target/FreeBSD/Thread.h"
#include "DebugServer2/Host/FreeBSD/PTrace.h"
#include "DebugServer2/Host/FreeBSD/ProcStat.h"
#include "DebugServer2/Utils/Log.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <DebugServer2/Architecture/X86_64/CPUState.h>

#define super ds2::Target::POSIX::Thread

namespace ds2 {
namespace Target {
namespace FreeBSD {

using Host::FreeBSD::ProcStat;

Thread::Thread(Process *process, ThreadId tid) : super(process, tid) {
  //
  // Initially the thread is stopped.
  //
  _state = kStopped;
  _lastSyscallNumber = -1;
}

Thread::~Thread() {}

ErrorCode Thread::terminate() {
  return process()->ptrace().kill(ProcessThreadId(process()->pid(), tid()),
                                  SIGKILL);
}

ErrorCode Thread::suspend() {
  struct ptrace_lwpinfo lwpinfo;
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
      DS2LOG(Target, Error, "failed to wait for tid %d, error=%s\n", tid(),
             strerror(errno));
      return error;
    }

    updateTrapInfo(status);
  }

  if (_state == kTerminated) {
    error = kErrorProcessNotFound;
  }

  return error;
}

ErrorCode Thread::step(int signal, Address const &address) {
  ErrorCode error = kSuccess;
  if (_state == kStopped || _state == kStepped) {
    DS2LOG(Target, Debug, "stepping tid %d", tid());
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

ErrorCode Thread::updateTrapInfo(int waitStatus) {
  struct ptrace_lwpinfo lwpinfo;

  super::updateTrapInfo(waitStatus);

  if (process()->_ptrace.getLwpInfo(process()->pid(), &lwpinfo) != kSuccess)
    return kSuccess;

  // Check for syscall entry
  if (lwpinfo.pl_flags & PL_FLAG_SCE) {
    Architecture::CPUState state;
    readCPUState(state);
    _lastSyscallNumber = state.state64.gp.rax;
    _trap.event = TrapInfo::kEventNone;
    _trap.reason = TrapInfo::kReasonNone;

    if (_lastSyscallNumber == SYS_thr_exit) {
      _trap.event = TrapInfo::kEventNone;
      _trap.reason = TrapInfo::kReasonThreadExit;
      return kSuccess;
    }
  }

  // Check for syscall exit
  if (lwpinfo.pl_flags & PL_FLAG_SCX) {
    if (_lastSyscallNumber == SYS_thr_create ||_lastSyscallNumber == SYS_thr_new) {
      _trap.event = TrapInfo::kEventNone;
      _trap.reason = TrapInfo::kReasonThreadNew;
      return kSuccess;
    }

    _trap.event = TrapInfo::kEventNone;
    _trap.reason = TrapInfo::kReasonNone;
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
  _trap.core = cpu;

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
}
}
}
