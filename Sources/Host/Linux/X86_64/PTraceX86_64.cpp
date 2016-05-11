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

static inline void user_to_state32(ds2::Architecture::X86_64::CPUState32 &state,
                                   user_fpregs_struct const &user) {
  //
  // X87 State
  //
  state.x87.fstw = user.swd;
  state.x87.fctw = user.cwd;
  state.x87.ftag = user.ftw;
  state.x87.fop = user.fop;
  state.x87.fiseg = user.rip >> 32;
  state.x87.fioff = user.rip;
  state.x87.foseg = user.rdp >> 32;
  state.x87.fooff = user.rdp;

  uint8_t const *st_space = reinterpret_cast<uint8_t const *>(user.st_space);
  for (size_t n = 0; n < 8; n++) {
    memcpy(state.x87.regs[n].bytes, st_space + n * 16,
           sizeof(state.x87.regs[n].bytes));
  }

  //
  // SSE State
  //
  state.sse.mxcsr = user.mxcsr;
  state.sse.mxcsrmask = user.mxcr_mask;
  uint8_t const *xmm_space = reinterpret_cast<uint8_t const *>(user.xmm_space);
  for (size_t n = 0; n < 8; n++) {
    memcpy(&state.sse.regs[n], xmm_space + n * 16, sizeof(state.sse.regs[n]));
  }
}

static inline void
state32_to_user(user_fpregs_struct &user,
                ds2::Architecture::X86_64::CPUState32 const &state) {
  //
  // X87 State
  //
  user.swd = state.x87.fstw;
  user.cwd = state.x87.fctw;
  user.ftw = state.x87.ftag;
  user.fop = state.x87.fop;
  user.rip = (static_cast<uint64_t>(state.x87.fiseg) << 32) | state.x87.fioff;
  user.rdp = (static_cast<uint64_t>(state.x87.foseg) << 32) | state.x87.fooff;

  uint8_t *st_space = reinterpret_cast<uint8_t *>(user.st_space);
  for (size_t n = 0; n < 8; n++) {
    memcpy(st_space + n * 16, state.x87.regs[n].bytes,
           sizeof(state.x87.regs[n].bytes));
  }

  //
  // SSE State
  //
  user.mxcsr = state.sse.mxcsr;
  user.mxcr_mask = state.sse.mxcsrmask;
  uint8_t *xmm_space = reinterpret_cast<uint8_t *>(user.xmm_space);
  for (size_t n = 0; n < 8; n++) {
    memcpy(xmm_space + n * 16, &state.sse.regs[n], sizeof(state.sse.regs[n]));
  }
}

//
// 64-bit helpers
//

static inline void user_to_state64(ds2::Architecture::X86_64::CPUState64 &state,
                                   user_fpregs_struct const &user) {
  //
  // X87 State
  //
  state.x87.fstw = user.swd;
  state.x87.fctw = user.cwd;
  state.x87.ftag = user.ftw;
  state.x87.fop = user.fop;
  state.x87.firip = user.rip;
  state.x87.forip = user.rdp;

  uint8_t const *st_space = reinterpret_cast<uint8_t const *>(user.st_space);
  for (size_t n = 0; n < 8; n++) {
    memcpy(state.x87.regs[n].bytes, st_space + n * 16,
           sizeof(state.x87.regs[n].bytes));
  }

  //
  // SSE State
  //
  state.sse.mxcsr = user.mxcsr;
  state.sse.mxcsrmask = user.mxcr_mask;
  uint8_t const *xmm_space = reinterpret_cast<uint8_t const *>(user.xmm_space);
  for (size_t n = 0; n < 16; n++) {
    memcpy(&state.sse.regs[n], xmm_space + n * 16, sizeof(state.sse.regs[n]));
  }
}

static inline void
state64_to_user(user_fpregs_struct &user,
                ds2::Architecture::X86_64::CPUState64 const &state) {
  //
  // X87 State
  //
  user.swd = state.x87.fstw;
  user.cwd = state.x87.fctw;
  user.ftw = state.x87.ftag;
  user.fop = state.x87.fop;
  user.rip = state.x87.firip;
  user.rdp = state.x87.forip;

  uint8_t *st_space = reinterpret_cast<uint8_t *>(user.st_space);
  for (size_t n = 0; n < 8; n++) {
    memcpy(st_space + n * 16, state.x87.regs[n].bytes,
           sizeof(state.x87.regs[n].bytes));
  }

  //
  // SSE State
  //
  user.mxcsr = state.sse.mxcsr;
  user.mxcr_mask = state.sse.mxcsrmask;
  uint8_t *xmm_space = reinterpret_cast<uint8_t *>(user.xmm_space);
  for (size_t n = 0; n < 16; n++) {
    memcpy(xmm_space + n * 16, &state.sse.regs[n], sizeof(state.sse.regs[n]));
  }
}

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
  // Read X87 and SSE state
  //
  user_fpregs_struct fprs;
  if (wrapPtrace(PTRACE_GETFPREGS, pid, nullptr, &fprs) == 0) {
    if (pinfo.pointerSize == sizeof(uint32_t)) {
      user_to_state32(state.state32, fprs);
    } else {
      user_to_state64(state.state64, fprs);
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
  // Write X87 and SSE state
  //
  user_fpregs_struct fprs;
  if (state.is32) {
    state32_to_user(fprs, state.state32);
  } else {
    state64_to_user(fprs, state.state64);
  }

  wrapPtrace(PTRACE_SETFPREGS, pid, nullptr, &fprs);

  return kSuccess;
}
}
}
}
