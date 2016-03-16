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

#define super ds2::Host::POSIX::PTrace

#include <asm/ptrace.h>
#include <elf.h>
#include <sys/ptrace.h>
#include <sys/uio.h>

namespace ds2 {
namespace Host {
namespace Linux {

struct PTracePrivateData {
  uint8_t breakpointCount;
  uint8_t watchpointCount;

  PTracePrivateData() : breakpointCount(0), watchpointCount(0) {}
};

void PTrace::initCPUState(ProcessId pid) {
  if (_privateData != nullptr)
    return;

  _privateData = new PTracePrivateData;

  // Retrieve the information about Hardware Breakpoint, if supported.
  // user_hwdebug_state.dbg_info is formatted as follows:
  //
  // 31             24             16               8              0
  // +---------------+--------------+---------------+---------------+
  // |   RESERVED    |   RESERVED   |   DEBUG_ARCH  |  NUM_SLOTS    |
  // +---------------+--------------+---------------+---------------+
  //
  // DEBUG_ARCH should be 0x06 for aarch64-armv8a.

  struct user_hwdebug_state drs;

  if (readRegisterSet(pid, NT_ARM_HW_BREAK, &drs, sizeof(drs)) != kSuccess ||
      ((drs.dbg_info >> 8) & 0xff) != 0x06)
    return;
  _privateData->breakpointCount = drs.dbg_info & 0xff;

  if (readRegisterSet(pid, NT_ARM_HW_WATCH, &drs, sizeof(drs)) != kSuccess ||
      ((drs.dbg_info >> 8) & 0xff) != 0x06)
    return;
  _privateData->watchpointCount = drs.dbg_info & 0xff;

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

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  pid_t pid;

  ErrorCode error = ptidToPid(ptid, pid);
  if (error != kSuccess)
    return error;

  // Initialize the CPU state, just in case.
  initCPUState(pid);

  state.isA32 = pinfo.pointerSize == sizeof(uint32_t);

  // Read GPRs.
  struct user_pt_regs gprs;
  error = readRegisterSet(ptid, NT_PRSTATUS, &gprs, sizeof(gprs));
  if (error != kSuccess)
    return error;

  // The layout is identical.
  std::memcpy(state.state64.gp.regs, &gprs, sizeof(state.state64.gp.regs));

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &,
                                Architecture::CPUState const &state) {
  pid_t pid;

  ErrorCode error = ptidToPid(ptid, pid);
  if (error != kSuccess)
    return error;

  // Initialize the CPU state, just in case.
  initCPUState(pid);

  // Write GPRs.
  struct user_pt_regs gprs;

  // The layout is identical.
  std::memcpy(&gprs, state.state64.gp.regs, sizeof(state.state64.gp.regs));

  error = writeRegisterSet(ptid, NT_PRSTATUS, &gprs, sizeof(gprs));
  if (error != kSuccess)
    return error;

  return kSuccess;
}
}
}
}
