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
                                   struct xfpregs_struct const &xfpregs) {
  //
  // X87 State
  //
  state.x87.fstw = xfpregs.fpregs.swd;
  state.x87.fctw = xfpregs.fpregs.cwd;
  state.x87.ftag = xfpregs.fpregs.ftw;
  state.x87.fop = xfpregs.fpregs.fop;
  state.x87.fiseg = xfpregs.fpregs.rip >> 32;
  state.x87.fioff = xfpregs.fpregs.rip;
  state.x87.foseg = xfpregs.fpregs.rdp >> 32;
  state.x87.fooff = xfpregs.fpregs.rdp;

  auto st_space = reinterpret_cast<uint8_t const *>(xfpregs.fpregs.st_space);
  static const size_t x87Size = sizeof(state.x87.regs[0].bytes);
  for (size_t n = 0; n < array_sizeof(state.x87.regs); n++) {
    memcpy(state.x87.regs[n].bytes, st_space + n * x87Size, x87Size);
  }

  //
  // SSE State
  //
  state.sse.mxcsr = xfpregs.fpregs.mxcsr;
  state.sse.mxcsrmask = xfpregs.fpregs.mxcr_mask;
  auto xmm_space = reinterpret_cast<uint8_t const *>(xfpregs.fpregs.xmm_space);
  static const size_t sseSize = sizeof(state.sse.regs[0]);
  for (size_t n = 0; n < array_sizeof(state.sse.regs); n++) {
    memcpy(&state.sse.regs[n], xmm_space + n * sseSize, sseSize);
  }

  //
  //  EAVX State
  //
  auto ymmh = reinterpret_cast<uint8_t const *>(xfpregs.ymmh);
  static const size_t avxSize = sizeof(state.avx.regs[0]);
  static const size_t ymmhSize = avxSize - sseSize;
  for (size_t n = 0; n < array_sizeof(state.avx.regs); n++) {
    auto avxHigh = reinterpret_cast<uint8_t *>(&state.avx.regs[n]) + sseSize;
    memcpy(avxHigh, ymmh + n * ymmhSize, ymmhSize);
  }
}

//
// 64-bit helpers
//

static inline void user_to_state64(ds2::Architecture::X86_64::CPUState64 &state,
                                   struct xfpregs_struct const &xfpregs) {
  //
  // X87 State
  //
  state.x87.fstw = xfpregs.fpregs.swd;
  state.x87.fctw = xfpregs.fpregs.cwd;
  state.x87.ftag = xfpregs.fpregs.ftw;
  state.x87.fop = xfpregs.fpregs.fop;
  state.x87.firip = xfpregs.fpregs.rip;
  state.x87.forip = xfpregs.fpregs.rdp;

  auto st_space = reinterpret_cast<uint8_t const *>(xfpregs.fpregs.st_space);
  static const size_t x87Size = sizeof(state.x87.regs[0].bytes);
  for (size_t n = 0; n < array_sizeof(state.x87.regs); n++) {
    memcpy(state.x87.regs[n].bytes, st_space + n * x87Size, x87Size);
  }

  //
  // SSE State
  //

  // This hack is necessary because we don't handle EAVX registers
  size_t numSSEState = array_sizeof(state.sse.regs);
  size_t numSSEUser =
      sizeof(xfpregs.fpregs.xmm_space) / sizeof(state.sse.regs[0]);
  size_t numSSE = std::min(numSSEState, numSSEUser);

  state.sse.mxcsr = xfpregs.fpregs.mxcsr;
  state.sse.mxcsrmask = xfpregs.fpregs.mxcr_mask;
  auto xmm_space = reinterpret_cast<uint8_t const *>(xfpregs.fpregs.xmm_space);
  static const size_t sseSize = sizeof(state.sse.regs[0]);
  for (size_t n = 0; n < numSSE; n++) {
    memcpy(&state.sse.regs[n], xmm_space + n * sseSize, sseSize);
  }

  //
  //  EAVX State
  //
  auto ymmh = reinterpret_cast<uint8_t const *>(xfpregs.ymmh);
  static const size_t avxSize = sizeof(state.avx.regs[0]);
  static const size_t ymmhSize = avxSize - sseSize;
  for (size_t n = 0; n < numSSE; n++) {
    auto avxHigh = reinterpret_cast<uint8_t *>(&state.avx.regs[n]) + sseSize;
    memcpy(avxHigh, ymmh + n * ymmhSize, ymmhSize);
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
      user_to_state32(state.state32, xfpregs);
    } else {
      user_to_state64(state.state64, xfpregs);
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
