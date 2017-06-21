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

#include <asm/ptrace.h>
#include <elf.h>
#include <sys/ptrace.h>
#include <sys/uio.h>

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace Linux {

int PTrace::getMaxStoppoints(ProcessThreadId const &ptid, int regSet) {
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
  if (readRegisterSet(ptid, regSet, &drs, sizeof(drs)) != kSuccess ||
      ((drs.dbg_info >> 8) & 0xff) != 0x06) {
    return 0;
  }

  return drs.dbg_info & 0xff;
}

int PTrace::getMaxHardwareBreakpoints(ProcessThreadId const &ptid) {
  return getMaxStoppoints(ptid, NT_ARM_HW_BREAK);
}

int PTrace::getMaxHardwareWatchpoints(ProcessThreadId const &ptid) {
  return getMaxStoppoints(ptid, NT_ARM_HW_WATCH);
}

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  state.isA32 = pinfo.pointerSize == sizeof(uint32_t);

  // Read GPRs.
  struct user_pt_regs gprs;
  CHK(readRegisterSet(ptid, NT_PRSTATUS, &gprs, sizeof(gprs)));
  // The layout is identical.
  std::memcpy(state.state64.gp.regs, &gprs, sizeof(state.state64.gp.regs));

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &,
                                Architecture::CPUState const &state) {
  // Write GPRs.
  struct user_pt_regs gprs;
  // The layout is identical.
  std::memcpy(&gprs, state.state64.gp.regs, sizeof(state.state64.gp.regs));
  CHK(writeRegisterSet(ptid, NT_PRSTATUS, &gprs, sizeof(gprs)));

  return kSuccess;
}
} // namespace Linux
} // namespace Host
} // namespace ds2
