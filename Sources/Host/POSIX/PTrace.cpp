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

ErrorCode PTrace::wait(ProcessThreadId const &ptid, int *status) {
  if (ptid.pid <= kAnyProcessId || !(ptid.tid <= kAnyThreadId))
    return kErrorInvalidArgument;

  int stat;
  pid_t wpid = ::waitpid(ptid.pid, &stat, 0);
  if (wpid != ptid.pid) {
    return Platform::TranslateError();
  }

  if (status != nullptr) {
    *status = stat;
  }

  return kSuccess;
}

ErrorCode PTrace::traceMe(bool disableASLR) {
  if (disableASLR) {
    DS2LOG(Warning, "disabling ASLR not implemented on current plaform");
  }

#if defined(OS_LINUX)
  auto cmd = PTRACE_TRACEME;
#elif defined(OS_FREEBSD) || defined(OS_DARWIN)
  auto cmd = PT_TRACE_ME;
#endif

  if (wrapPtrace(cmd, 0, nullptr, nullptr) < 0)
    return Platform::TranslateError();

  return kSuccess;
}

ErrorCode PTrace::attach(ProcessId pid) {
  if (pid <= kAnyProcessId)
    return kErrorProcessNotFound;

  DS2LOG(Debug, "attaching to pid %" PRIu64, (uint64_t)pid);

#if defined(OS_LINUX) || defined(OS_FREEBSD)
  auto cmd = PTCMD(ATTACH);
#elif defined(OS_DARWIN)
  auto cmd = PT_ATTACHEXC;
#endif

  if (wrapPtrace(cmd, pid, nullptr, nullptr) < 0)
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

ErrorCode PTrace::suspend(ProcessThreadId const &ptid) {
  // This will call PTrace::kill, not the kill(2) system call.
  return kill(ptid, SIGSTOP);
}

ErrorCode PTrace::doStepResume(bool stepping, ProcessThreadId const &ptid,
                               int signal, Address const &address) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

#if defined(OS_LINUX)
  // Handling of continuation address is performed in Linux::PTrace.
  DS2ASSERT(!address.valid());
  auto res = wrapPtrace(stepping ? PTRACE_SINGLESTEP : PTRACE_CONT, pid,
                        nullptr, signal);
#elif defined(OS_FREEBSD) || defined(OS_DARWIN)
  // (caddr_t)1 indicates that execution is to pick up where it left off.
  auto res = wrapPtrace(stepping ? PT_STEP : PT_CONTINUE, pid,
                        address.valid() ? address.value() : 1, signal);
#endif
  if (res < 0) {
    return Platform::TranslateError();
  }

  return kSuccess;
}

ErrorCode PTrace::step(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                       int signal, Address const &address) {
  return doStepResume(true, ptid, signal, address);
}

ErrorCode PTrace::resume(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                         int signal, Address const &address) {
  return doStepResume(false, ptid, signal, address);
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

ErrorCode PTrace::ptidToPid(ProcessThreadId const &ptid, pid_t &pid) {
  if (!ptid.valid())
    return kErrorInvalidArgument;

  pid = (ptid.tid <= kAnyThreadId) ? ptid.pid : ptid.tid;

  return kSuccess;
}
} // namespace POSIX
} // namespace Host
} // namespace ds2
