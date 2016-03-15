//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/POSIX/PTrace.h"
#include "DebugServer2/Host/Platform.h"

#include <cerrno>
#include <sys/wait.h>

namespace ds2 {
namespace Host {
namespace POSIX {

#if defined(OS_LINUX)
#define PTCMD(CMD) PTRACE_##CMD
#elif defined(OS_FREEBSD) || defined(OS_DARWIN)
#define PTCMD(CMD) PT_##CMD
#endif

PTrace::PTrace() = default;

PTrace::~PTrace() = default;

ErrorCode PTrace::wait(ProcessThreadId const &ptid, int *status) {
  if (ptid.pid <= kAnyProcessId || !(ptid.tid <= kAnyThreadId))
    return kErrorInvalidArgument;

  int stat;
  pid_t wpid = ::waitpid(ptid.pid, &stat, 0);
  if (wpid != ptid.pid) {
    switch (errno) {
    case ESRCH:
      return kErrorProcessNotFound;
    default:
      return kErrorInvalidArgument;
    }
  }

  if (status != nullptr) {
    *status = stat;
  }

  return kSuccess;
}

ErrorCode PTrace::attach(ProcessId pid) {
  if (pid <= kAnyProcessId)
    return kErrorProcessNotFound;

  DS2LOG(Debug, "attaching to pid %" PRIu64, (uint64_t)pid);

  if (wrapPtrace(PTCMD(ATTACH), pid, nullptr, nullptr) < 0)
    return Platform::TranslateError();

  return kSuccess;
}

ErrorCode PTrace::detach(ProcessId pid) {
  if (pid <= kAnyProcessId)
    return kErrorProcessNotFound;

  DS2LOG(Debug, "detaching from pid %" PRIu64, (uint64_t)pid);

  if (wrapPtrace(PTCMD(DETACH), pid, nullptr, nullptr) < 0)
    return Platform::TranslateError();

  return kSuccess;
}

//
// Execute will execute the code in the target process/thread,
// the techinique for PTRACE is to read CPU state, rewrite the
// code portion pointed by PC with the code, resuming the process/thread,
// wait for the completion, read the CPU state back to grab return
// values, then restoring the previous code.
//
ErrorCode PTrace::execute(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                          void const *code, size_t length, uint64_t &result) {
  Architecture::CPUState savedState, resultState;
  std::string savedCode;

  if (!ptid.valid() || code == nullptr || length == 0)
    return kErrorInvalidArgument;

  // 1. Read and save the CPU state
  ErrorCode error = readCPUState(ptid, pinfo, savedState);
  if (error != kSuccess)
    return error;

  // 2. Copy the code at PC
  savedCode.resize(length);
  error = readMemory(ptid, savedState.pc(), &savedCode[0], length);
  if (error != kSuccess)
    return error;

  // 3. Write the code to execute at PC
  error = writeMemory(ptid, savedState.pc(), code, length);
  if (error != kSuccess)
    goto fail;

  // 4. Resume and wait
  error = resume(ptid, pinfo);
  if (error == kSuccess) {
    error = wait(ptid);
  }

  if (error == kSuccess) {
    // 5. Read back the CPU state
    error = readCPUState(ptid, pinfo, resultState);
    if (error == kSuccess) {
      // 6. Save the result
      result = resultState.retval();
    }
  }

  // 7. Write back the old code
  error = writeMemory(ptid, savedState.pc(), &savedCode[0], length);
  if (error != kSuccess)
    goto fail;

  // 8. Restore CPU state
  error = writeCPUState(ptid, pinfo, savedState);
  if (error != kSuccess)
    goto fail;

  // Success!! We injected and executed code!
  return kSuccess;

fail:
  kill(ptid, SIGKILL); // we can't really do much at this point :(
  return error;
}

ErrorCode PTrace::getEventPid(ProcessThreadId const &ptid, ProcessId &pid) {
  return kErrorUnsupported;
}

ErrorCode PTrace::ptidToPid(ProcessThreadId const &ptid, pid_t &pid) {
  if (!ptid.valid())
    return kErrorInvalidArgument;

  pid = (ptid.tid <= kAnyThreadId) ? ptid.pid : ptid.tid;

  return kSuccess;
}
}
}
}
