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
#include "DebugServer2/Core/HardwareBreakpointManager.h"
#include "DebugServer2/Host/Linux/ExtraWrappers.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"
#include "DebugServer2/Utils/Stringify.h"

#include <asm/ptrace.h>
#include <elf.h>
#include <sys/ptrace.h>
#include <sys/uio.h>

using ds2::Utils::Stringify;

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace Linux {

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid, ProcessInfo const &,
                               Architecture::CPUState &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

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

  return kSuccess;
}

uint32_t PTrace::getStoppointData(ProcessThreadId const &ptid) {
  pid_t pid;
  ErrorCode error = ptidToPid(ptid, pid);
  if (error != kSuccess) {
    return 0;
  }

  //
  // Retrieve the information about Hardware Breakpoint, if supported.
  //
  unsigned int value = 0;
  if (wrapPtrace(PTRACE_GETHBPREGS, pid, 0, &value) < 0) {
    DS2LOG(Debug, "hardware breakpoint support disabled, error=%s",
           Stringify::Errno(errno));
    return 0;
  }

  if (((value >> 24) & 0xff) == 0) {
    return 0;
  }

  return value;
}

int PTrace::getMaxHardwareBreakpoints(ProcessThreadId const &ptid) {
  return getStoppointData(ptid) & 0xff;
}

int PTrace::getMaxHardwareWatchpoints(ProcessThreadId const &ptid) {
  return (getStoppointData(ptid) >> 8) & 0xff;
}

int PTrace::getMaxWatchpointSize(ProcessThreadId const &ptid) {
  return (getStoppointData(ptid) >> 16) & 0xff;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &,
                                Architecture::CPUState const &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

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

  return kSuccess;
}

ErrorCode PTrace::writeStoppoint(ProcessThreadId const &ptid, size_t idx,
                                 uint32_t *val) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  if (wrapPtrace(PTRACE_SETHBPREGS, pid, idx, val) < 0)
    return Platform::TranslateError();

  return kSuccess;
}

ErrorCode PTrace::writeHardwareBreakpoint(ProcessThreadId const &ptid,
                                          uint32_t addr, uint32_t ctrl,
                                          size_t idx) {
  ErrorCode error;

  error = writeStoppoint(ptid, (idx << 1) + 1, &addr);
  if (error != kSuccess)
    return error;

  error = writeStoppoint(ptid, (idx << 1) + 2, &ctrl);
  if (error != kSuccess)
    return error;

  return kSuccess;
}

ErrorCode PTrace::writeHardwareWatchpoint(ProcessThreadId const &ptid,
                                          uint32_t addr, uint32_t ctrl,
                                          size_t idx) {
  ErrorCode error;

  error = writeStoppoint(ptid, -((idx << 1) + 1), &addr);
  if (error != kSuccess)
    return error;

  error = writeStoppoint(ptid, -((idx << 1) + 2), &ctrl);
  if (error != kSuccess)
    return error;

  return kSuccess;
}
} // namespace Linux
} // namespace Host
} // namespace ds2
