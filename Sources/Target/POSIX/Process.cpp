//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#define __DS2_LOG_CLASS_NAME__ "Target::Process"

#include "DebugServer2/Target/Process.h"
#include "DebugServer2/BreakpointManager.h"
#include "DebugServer2/Host/POSIX/PTrace.h"
#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Utils/Log.h"

#include <cerrno>
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>

using ds2::Host::ProcessSpawner;

#define super ds2::Target::ProcessBase

namespace ds2 {
namespace Target {
namespace POSIX {

ErrorCode Process::initialize(ProcessId pid, uint32_t flags) {
  ErrorCode error;

  // Wait the main thread.
  int status;
  error = ptrace().wait(pid, &status);
  if (error != kSuccess) {
    return error;
  }

  error = ptrace().traceThat(pid);
  if (error != kSuccess) {
    return error;
  }

  error = super::initialize(pid, flags);
  if (error != kSuccess) {
    return error;
  }

  return attach(status);
}

ErrorCode Process::detach() {
  prepareForDetach();

  ErrorCode error = ptrace().detach(_pid);
  if (error != kSuccess) {
    return error;
  }

  cleanup();
  _flags &= ~kFlagAttachedProcess;

  return kSuccess;
}

ErrorCode Process::interrupt() { return ptrace().kill(_pid, SIGSTOP); }

ErrorCode Process::terminate() {
  // We have to use SIGKILL here, for two (related) reasons:
  // (1) we want to make sure the tracee can't catch the signal when we are
  //     trying to terminate it;
  // (2) using SIGKILL allows us to send the signal with a simple `kill()`
  //     instead of having to use ptrace(PTRACE_CONT, ...)`.
  return ptrace().kill(_pid, SIGKILL);
}

bool Process::isAlive() const { return (_pid > 0 && ::kill(_pid, 0) == 0); }

ErrorCode Process::readString(Address const &address, std::string &str,
                              size_t length, size_t *count) {
  auto id = _currentThread == nullptr ? _pid : _currentThread->tid();
  return ptrace().readString(id, address, str, length, count);
}

ErrorCode Process::readMemory(Address const &address, void *data, size_t length,
                              size_t *count) {
  auto id = _currentThread == nullptr ? _pid : _currentThread->tid();
  return ptrace().readMemory(id, address, data, length, count);
}

ErrorCode Process::writeMemory(Address const &address, void const *data,
                               size_t length, size_t *count) {
  auto id = _currentThread == nullptr ? _pid : _currentThread->tid();
  return ptrace().writeMemory(id, address, data, length, count);
}

ErrorCode Process::wait() { return ptrace().wait(_pid, nullptr); }

ds2::Target::Process *Process::Attach(ProcessId pid) {
  if (pid <= 0)
    return nullptr;

  // Create the process.
  auto process = new Target::Process;

  // And try to attach.
  ErrorCode error = process->ptrace().attach(pid);
  if (error != kSuccess) {
    DS2LOG(Error, "ptrace attach failed: %s", strerror(errno));
    goto fail;
  }

  error = process->initialize(pid, kFlagAttachedProcess);
  if (error != kSuccess) {
    process->ptrace().detach(pid);
    goto fail;
  }

  return process;

fail:
  delete process;
  return nullptr;
}

ds2::Target::Process *Process::Create(ProcessSpawner &spawner) {
  ErrorCode error;
  pid_t pid;

  // Create the process.
  auto process = new Target::Process;

  error = spawner.run(
      [process]() { return process->ptrace().traceMe(true) == kSuccess; });
  if (error != kSuccess)
    goto fail;

  pid = spawner.pid();
  DS2LOG(Debug, "created process %d", pid);

  // Wait the process.
  error = process->initialize(pid, kFlagNewProcess);
  if (error != kSuccess)
    goto fail;

  // Give a chance to the ptracer to set any specific options.
  error = process->ptrace().traceThat(pid);
  if (error != kSuccess)
    goto fail;

  return process;

fail:
  delete process;
  return nullptr;
}

void Process::resetSignalPass() {
  _passthruSignals.clear();
  _passthruSignals.insert(SIGINT);
}

void Process::setSignalPass(int signo, bool set) {
  if (set) {
    _passthruSignals.insert(signo);
  } else {
    _passthruSignals.erase(signo);
  }
}
}
}
}
