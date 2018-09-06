//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Core/BreakpointManager.h"
#include "DebugServer2/Host/POSIX/PTrace.h"
#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Utils/Log.h"

#include <cerrno>
#include <csignal>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

using ds2::Host::ProcessSpawner;

#define super ds2::Target::ProcessBase

namespace ds2 {
namespace Target {
namespace POSIX {

ErrorCode Process::initialize(ProcessId pid, uint32_t flags) {
  // Wait the main thread.
  int status;
  CHK(ptrace().wait(pid, &status));
  CHK(ptrace().traceThat(pid));

  // Can't use `CHK()` here because of a bug in GCC. See
  // http://stackoverflow.com/a/12086873/5731788
  ErrorCode error = super::initialize(pid, flags);
  if (error != kSuccess) {
    return error;
  }

  CHK(attach(status));

  return kSuccess;
}

ErrorCode Process::detach() {
  prepareForDetach();

  CHK(ptrace().detach(_pid));

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

int Process::convertMemoryProtectionToPOSIX(uint32_t protection) const {
  int res = PROT_NONE;

  if (protection & kProtectionRead) {
    res |= PROT_READ;
  }
  if (protection & kProtectionWrite) {
    res |= PROT_WRITE;
  }
  if (protection & kProtectionExecute) {
    res |= PROT_EXEC;
  }

  return res;
}

uint32_t Process::convertMemoryProtectionFromPOSIX(int POSIXProtection) const {
  uint32_t res = kProtectionNone;

  if (POSIXProtection & PROT_READ) {
    res |= kProtectionRead;
  }
  if (POSIXProtection & PROT_WRITE) {
    res |= kProtectionWrite;
  }
  if (POSIXProtection & PROT_EXEC) {
    res |= kProtectionExecute;
  }

  return res;
}

ErrorCode Process::wait() { return ptrace().wait(_pid, nullptr); }

ds2::Target::Process *Process::Attach(ProcessId pid) {
  if (pid <= 0)
    return nullptr;

  auto process = ds2::make_unique<Target::Process>();

  if (process->ptrace().attach(pid) != kSuccess) {
    DS2LOG(Error, "ptrace attach failed: %s", strerror(errno));
    return nullptr;
  }

  if (process->initialize(pid, kFlagAttachedProcess) != kSuccess) {
    process->ptrace().detach(pid);
    return nullptr;
  }

  return process.release();
}

ds2::Target::Process *Process::Create(ProcessSpawner &spawner) {
  auto process = ds2::make_unique<Target::Process>();

  if (spawner.run([&process]() {
        return process->ptrace().traceMe(true) == kSuccess;
      }) != kSuccess) {
    return nullptr;
  }

  pid_t pid = spawner.pid();
  DS2LOG(Debug, "created process %d", pid);

  if (process->initialize(pid, kFlagNewProcess) != kSuccess) {
    return nullptr;
  }

  // Give a chance to the ptrace implementation to set any specific options.
  if (process->ptrace().traceThat(pid) != kSuccess) {
    return nullptr;
  }

  return process.release();
}

void Process::resetSignalPass() { _passthruSignals.clear(); }

void Process::setSignalPass(int signo, bool set) {
  if (set) {
    _passthruSignals.insert(signo);
  } else {
    _passthruSignals.erase(signo);
  }
}
} // namespace POSIX
} // namespace Target
} // namespace ds2
