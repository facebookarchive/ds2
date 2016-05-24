//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/Linux/PTrace.h"
#include "DebugServer2/Architecture/X86/RegisterCopy.h"
#include "DebugServer2/Host/Linux/ExtraWrappers.h"
#include "DebugServer2/Host/Platform.h"

#include <elf.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/user.h>

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace Linux {

struct PTracePrivateData {
  uint8_t breakpointCount;
  uint8_t watchpointCount;
  uint8_t maxWatchpointSize;

  PTracePrivateData()
      : breakpointCount(0), watchpointCount(0), maxWatchpointSize(0) {}
};

void PTrace::initCPUState(ProcessId pid) {
  if (_privateData != nullptr)
    return;

  _privateData = new PTracePrivateData;
}

void PTrace::doneCPUState() { delete _privateData; }

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  pid_t pid;

  ErrorCode error = ptidToPid(ptid, pid);
  if (error != kSuccess)
    return error;

  //
  // Initialize the CPU state, just in case.
  //
  initCPUState(pid);

  //
  // Read GPRs
  //
  user_regs_struct gprs;
  if (wrapPtrace(PTRACE_GETREGS, pid, nullptr, &gprs) < 0)
    return Platform::TranslateError();

  if (pinfo.pointerSize == sizeof(uint32_t)) {
    state.is32 = true;
    Architecture::X86::user_to_state32(state.state32, gprs);
  } else {
    state.is32 = false;
    Architecture::X86::user_to_state64(state.state64, gprs);
  }

  //
  // Read SSE and AVX
  //
  struct xfpregs_struct xfpregs;
  struct iovec fpregs_iovec;
  fpregs_iovec.iov_base = &xfpregs;
  fpregs_iovec.iov_len = sizeof(xfpregs);

  // If this call fails, don't return failure, since AVX may not be available
  // on this CPU
  if (wrapPtrace(PTRACE_GETREGSET, pid, NT_X86_XSTATE, &fpregs_iovec) == 0) {
    if (pinfo.pointerSize == sizeof(uint32_t)) {
      Architecture::X86::user_to_state32(state.state32, xfpregs);
    } else {
      Architecture::X86::user_to_state64(state.state64, xfpregs);
    }
  }

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &pinfo,
                                Architecture::CPUState const &state) {
  pid_t pid;

  ErrorCode error = ptidToPid(ptid, pid);
  if (error != kSuccess)
    return error;

  //
  // Initialize the CPU state, just in case.
  //
  initCPUState(pid);

  if (pinfo.pointerSize == sizeof(uint32_t) && !state.is32)
    return kErrorInvalidArgument;
  else if (pinfo.pointerSize != sizeof(uint32_t) && state.is32)
    return kErrorInvalidArgument;

  //
  // Write GPRs
  //
  user_regs_struct gprs;
  if (state.is32) {
    Architecture::X86::state32_to_user(gprs, state.state32);
  } else {
    Architecture::X86::state64_to_user(gprs, state.state64);
  }

  if (wrapPtrace(PTRACE_SETREGS, pid, nullptr, &gprs) < 0)
    return Platform::TranslateError();

  //
  // Write SSE and AVX
  //
  struct xfpregs_struct xfpregs;
  struct iovec fpregs_iovec;
  fpregs_iovec.iov_base = &xfpregs;
  fpregs_iovec.iov_len = sizeof(xfpregs);

  // We need this read to fill the kernel header (xfpregs_struct.xstate_hdr)
  // TODO - add this header to CPUState so we don't need this read
  if (wrapPtrace(PTRACE_GETREGSET, pid, NT_X86_XSTATE, &fpregs_iovec) < 0) {
    // If we fail to read the AVX register info, still write the SSE regs
    fpregs_iovec.iov_len = sizeof(xfpregs.fpregs);
  }

  if (state.is32) {
    Architecture::X86::state32_to_user(xfpregs, state.state32);
  } else {
    Architecture::X86::state64_to_user(xfpregs, state.state64);
  }

  wrapPtrace(PTRACE_SETREGSET, pid, NT_X86_XSTATE, &fpregs_iovec);

  return kSuccess;
}
}
}
}
