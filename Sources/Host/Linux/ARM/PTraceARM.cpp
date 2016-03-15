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
#include "DebugServer2/Host/Linux/ExtraWrappers.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Support/Stringify.h"
#include "DebugServer2/Utils/Log.h"

#define super ds2::Host::POSIX::PTrace

#include <asm/ptrace.h>
#include <elf.h>
#include <sys/ptrace.h>
#include <sys/uio.h>

using ds2::Support::Stringify;

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

  //
  // Retrieve the information about Hardware Breakpoint, if supported.
  //
  unsigned int value = 0;
  if (wrapPtrace(PTRACE_GETHBPREGS, pid, 0, &value) < 0) {
    DS2LOG(Debug, "hardware breakpoint support disabled, error=%s",
           Stringify::Errno(errno));
    return;
  }

  if ((value >> 24) == 0)
    return;

  _privateData->breakpointCount = value & 0xff;
  _privateData->watchpointCount = (value >> 8) & 0xff;
  _privateData->maxWatchpointSize = (value >> 16) & 0xff;

  //
  // Set our hard limits.
  //
  if (_privateData->breakpointCount > 32) {
    _privateData->breakpointCount = 32;
  }
  if (_privateData->watchpointCount > 32) {
    _privateData->watchpointCount = 32;
  }
}

void PTrace::doneCPUState() { delete _privateData; }

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid, ProcessInfo const &,
                               Architecture::CPUState &state) {
  pid_t pid;

  if (!ptid.valid())
    return kErrorInvalidArgument;

  if (!(ptid.tid <= kAnyThreadId)) {
    pid = ptid.tid;
  } else {
    pid = ptid.pid;
  }

  //
  // Initialize the CPU state, just in case.
  //
  initCPUState(pid);

  //
  // Read GPRs
  //
  struct pt_regs gprs;
  if (wrapPtrace(PTRACE_GETREGS, pid, nullptr, &gprs) < 0)
    return Platform::TranslateError();

  //
  // The layout is identical.
  //
  std::memcpy(state.gp.regs, gprs.uregs, sizeof(state.gp.regs));

  static_assert(sizeof(state.vfp) == ARM_VFPREGS_SIZE,
                "sizeof(ARM::CPUState.vfp) does not match ARM_VFPREGS_SIZE");

  if (wrapPtrace(PTRACE_GETVFPREGS, pid, nullptr, &state.vfp) < 0)
    return Platform::TranslateError();

  //
  // Read hardware breakpoints and watchpoints.
  //
  for (size_t n = 0; n < _privateData->breakpointCount; n++) {
    unsigned long value;

    if (wrapPtrace(PTRACE_GETHBPREGS, pid, (n << 1) + 1, &value) < 0) {
      state.hbp.bcr[n] = 0;
    } else {
      state.hbp.bcr[n] = value;
    }

    if (wrapPtrace(PTRACE_GETHBPREGS, pid, (n << 1) + 2, &value) < 0) {
      state.hbp.bvr[n] = 0;
    } else {
      state.hbp.bvr[n] = value;
    }
  }

  for (size_t n = 0; n < _privateData->watchpointCount; n++) {
    if (wrapPtrace(PTRACE_GETHBPREGS, pid, -((n << 1) + 1), &state.hbp.wcr[n]) <
        0) {
      state.hbp.wcr[n] = 0;
    }
    if (wrapPtrace(PTRACE_GETHBPREGS, pid, -((n << 1) + 2), &state.hbp.wvr[n]) <
        0) {
      state.hbp.wvr[n] = 0;
    }
  }

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &,
                                Architecture::CPUState const &state) {
  pid_t pid;

  if (!ptid.valid())
    return kErrorInvalidArgument;

  if (!(ptid.tid <= kAnyThreadId)) {
    pid = ptid.tid;
  } else {
    pid = ptid.pid;
  }

  //
  // Initialize the CPU state, just in case.
  //
  initCPUState(pid);

  //
  // Read GPRs
  //
  struct pt_regs gprs;

  //
  // The layout is identical.
  //
  std::memcpy(gprs.uregs, state.gp.regs, sizeof(state.gp.regs));
  gprs.ARM_ORIG_r0 = 0;

  if (wrapPtrace(PTRACE_SETREGS, pid, nullptr, &gprs) < 0)
    return Platform::TranslateError();

  static_assert(sizeof(state.vfp) == ARM_VFPREGS_SIZE,
                "sizeof(ARM::CPUState.vfp) does not match ARM_VFPREGS_SIZE");

  if (wrapPtrace(PTRACE_SETVFPREGS, pid, nullptr, &state.vfp) < 0)
    return Platform::TranslateError();

  //
  // Write hardware breakpoints and watchpoints.
  //
  for (size_t n = 0; n < _privateData->breakpointCount; n++) {
    wrapPtrace(PTRACE_SETHBPREGS, pid, (n << 1) + 1, &state.hbp.bcr[n]);
    wrapPtrace(PTRACE_GETHBPREGS, pid, (n << 1) + 2, &state.hbp.bvr[n]);
  }

  for (size_t n = 0; n < _privateData->watchpointCount; n++) {
    wrapPtrace(PTRACE_SETHBPREGS, pid, -((n << 1) + 1), &state.hbp.wcr[n]);
    wrapPtrace(PTRACE_SETHBPREGS, pid, -((n << 1) + 2), &state.hbp.wvr[n]);
  }

  return kSuccess;
}
}
}
}
