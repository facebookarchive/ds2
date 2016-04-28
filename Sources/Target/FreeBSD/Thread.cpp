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
                                     &status);
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
      switch (_stopInfo.signal) {
      case SIGCHLD:
      case SIGSTOP:
      case SIGTRAP:
        signal = 0;
        break;
      default:
        signal = _stopInfo.signal;
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
      _stopInfo.signal = 0;
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
}
}
}
