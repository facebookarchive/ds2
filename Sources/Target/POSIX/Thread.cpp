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
#if defined(ARCH_ARM)
#include "DebugServer2/Architecture/ARM/SoftwareSingleStep.h"
#endif
#include "DebugServer2/Target/Process.h"

#include <sys/wait.h>

#define super ds2::Target::ThreadBase

namespace ds2 {
namespace Target {
namespace POSIX {

Thread::Thread(ds2::Target::Process *process, ThreadId tid)
    : super(process, tid) {}

ErrorCode Thread::readCPUState(Architecture::CPUState &state) {
  // TODO cache CPU state
  ProcessInfo info;

  CHK(_process->getInfo(info));
  CHK(process()->ptrace().readCPUState(ProcessThreadId(process()->pid(), tid()),
                                       info, state));

  return kSuccess;
}

ErrorCode Thread::writeCPUState(Architecture::CPUState const &state) {
  ProcessInfo info;

  CHK(_process->getInfo(info));
  CHK(process()->ptrace().writeCPUState(
      ProcessThreadId(process()->pid(), tid()), info, state));

  return kSuccess;
}

ErrorCode Thread::terminate() {
  return process()->ptrace().kill(ProcessThreadId(process()->pid(), tid()),
                                  SIGKILL);
}

ErrorCode Thread::suspend() {
  if (_state == kRunning) {
    CHK(process()->ptrace().suspend(ProcessThreadId(process()->pid(), tid())));

    int status;
    ErrorCode error = process()->ptrace().wait(
        ProcessThreadId(process()->pid(), tid()), &status);
    if (error != kSuccess) {
      DS2LOG(Error, "failed to wait for tid %d, error=%s\n", tid(),
             strerror(errno));
      return error;
    }

    updateStopInfo(status);
  }

  if (_state == kTerminated) {
    return kErrorProcessNotFound;
  }

  return kSuccess;
}

#if defined(ARCH_ARM)
ErrorCode Thread::step(int signal, Address const &address) {
  if (_state == kInvalid || _state == kRunning) {
    return kErrorInvalidArgument;
  } else if (_state == kTerminated) {
    return kErrorProcessNotFound;
  }

  DS2LOG(Debug, "stepping tid %d", tid());

  // Prepare a software (arch-dependent) single step and resume execution.
  Architecture::CPUState state;
  CHK(readCPUState(state));
  CHK(PrepareSoftwareSingleStep(
      process(), process()->softwareBreakpointManager(), state, address));
  CHK(resume(signal, address));
  _state = kStepped;
  return kSuccess;
}
#else
ErrorCode Thread::step(int signal, Address const &address) {
  if (_state == kInvalid || _state == kRunning) {
    return kErrorInvalidArgument;
  } else if (_state == kTerminated) {
    return kErrorProcessNotFound;
  }

  DS2LOG(Debug, "stepping tid %d", tid());

  ProcessInfo info;
  CHK(process()->getInfo(info));
  CHK(process()->ptrace().step(ProcessThreadId(process()->pid(), tid()), info,
                               signal, address));
  _state = kStepped;
  return kSuccess;
}
#endif

ErrorCode Thread::resume(int signal, Address const &address) {
  if (_state == kStopped || _state == kStepped) {
    ProcessInfo info;

    CHK(process()->getInfo(info));
    CHK(process()->ptrace().resume(ProcessThreadId(process()->pid(), tid()),
                                   info, signal, address));
    _state = kRunning;
    _stopInfo.signal = 0;
  } else if (_state == kTerminated) {
    return kErrorProcessNotFound;
  }

  return kSuccess;
}

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
} // namespace POSIX
} // namespace Target
} // namespace ds2
