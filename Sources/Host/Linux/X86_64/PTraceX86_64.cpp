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

#include <cstddef>
#include <elf.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/user.h>

#define super ds2::Host::POSIX::PTrace

using ds2::Architecture::X86::XFeature;

namespace ds2 {
namespace Host {
namespace Linux {

static inline void user_to_state32(ds2::Architecture::X86_64::CPUState32 &state,
                                   struct xsave_struct const &xfpregs) {
  // Legacy (fxsave) registers
  state.x87.fctw = xfpregs.fpregs.fctw;
  state.x87.fstw = xfpregs.fpregs.fstw;
  state.x87.ftag = xfpregs.fpregs.ftag;
  state.x87.fop = xfpregs.fpregs.fop;
  state.x87.fioff = xfpregs.fpregs.fioff;
  state.x87.fiseg = xfpregs.fpregs.fiseg;
  state.x87.fooff = xfpregs.fpregs.fooff;
  state.x87.foseg = xfpregs.fpregs.foseg;

  auto st_space = reinterpret_cast<uint8_t const *>(xfpregs.fpregs.st_space);
  static const size_t x87DataSize = sizeof(state.x87.regs[0].data);
  static const size_t x87RegSize = x87DataSize + x87Padding;
  for (size_t n = 0; n < array_sizeof(state.x87.regs); n++) {
    memcpy(state.x87.regs[n].data, st_space + n * x87RegSize, x87DataSize);
  }

  state.sse.mxcsr = xfpregs.fpregs.mxcsr;
  state.sse.mxcsrmask = xfpregs.fpregs.mxcsrmask;
  auto xmm_space = reinterpret_cast<uint8_t const *>(xfpregs.fpregs.xmm_space);
  static const size_t sseRegSize = sizeof(state.sse.regs[0]);
  for (size_t n = 0; n < array_sizeof(state.sse.regs); n++) {
    memcpy(&state.sse.regs[n], xmm_space + n * sseRegSize, sseRegSize);
  }

  state.xcr0 = xfpregs.fpregs.xcr0;

  // XSAVE Header
  state.xsave_header.xfeatures_mask = xfpregs.header.xfeatures_mask;

  //
  //  EAVX State
  //
  auto ymmh = reinterpret_cast<uint8_t const *>(xfpregs.ymmh);
  static const size_t avxSize = sizeof(state.avx.regs[0]);
  static const size_t ymmhSize = avxSize - sseRegSize;
  for (size_t n = 0; n < array_sizeof(state.avx.regs); n++) {
    auto avxHigh = reinterpret_cast<uint8_t *>(&state.avx.regs[n]) + sseRegSize;
    memcpy(avxHigh, ymmh + n * ymmhSize, ymmhSize);
  }
}

static inline void
state32_to_user(struct xsave_struct &xfpregs,
                ds2::Architecture::X86_64::CPUState32 const &state) {
  // Legacy (fxsave) registers
  xfpregs.fpregs.fctw = state.x87.fctw;
  xfpregs.fpregs.fstw = state.x87.fstw;
  xfpregs.fpregs.ftag = state.x87.ftag;
  xfpregs.fpregs.fop = state.x87.fop;
  xfpregs.fpregs.fioff = state.x87.fioff;
  xfpregs.fpregs.fiseg = state.x87.fiseg;
  xfpregs.fpregs.fooff = state.x87.fooff;
  xfpregs.fpregs.foseg = state.x87.foseg;

  auto st_space = reinterpret_cast<uint8_t *>(xfpregs.fpregs.st_space);
  static const size_t x87DataSize = sizeof(state.x87.regs[0].data);
  static const size_t x87RegSize = x87DataSize + x87Padding;
  for (size_t n = 0; n < array_sizeof(state.x87.regs); n++) {
    memcpy(st_space + n * x87RegSize, state.x87.regs[n].data, x87DataSize);
  }

  xfpregs.fpregs.mxcsr = state.sse.mxcsr;
  xfpregs.fpregs.mxcsrmask = state.sse.mxcsrmask;
  auto xmm_space = reinterpret_cast<uint8_t *>(xfpregs.fpregs.xmm_space);
  static const size_t sseRegSize = sizeof(state.sse.regs[0]);
  for (size_t n = 0; n < array_sizeof(state.sse.regs); n++) {
    memcpy(xmm_space + n * sseRegSize, &state.sse.regs[n], sseRegSize);
  }

  xfpregs.fpregs.xcr0 = state.xcr0;

  // XSAVE Header
  xfpregs.header.xfeatures_mask = state.xsave_header.xfeatures_mask;
  if (state.xcr0 & XFeature::X86_X87) {
    xfpregs.header.xfeatures_mask |= XFeature::X86_X87;
  }
  if (state.xcr0 & XFeature::X86_SSE) {
    xfpregs.header.xfeatures_mask |= XFeature::X86_SSE;
  }
  if (state.xcr0 & XFeature::X86_AVX) {
    xfpregs.header.xfeatures_mask |= XFeature::X86_AVX;
  }
  // TODO: Support xcomp_bv. See 13.4.2 XSAVE Header in Intels' 64 and IA32
  // Architecture Manual to understand the purpose of xcomp_bv.
  xfpregs.header.xcomp_bv = 0;

  //
  //  EAVX State
  //
  auto ymmh = reinterpret_cast<uint8_t *>(xfpregs.ymmh);
  static const size_t avxSize = sizeof(state.avx.regs[0]);
  static const size_t ymmhSize = avxSize - sseRegSize;
  for (size_t n = 0; n < array_sizeof(state.avx.regs); n++) {
    auto avxHigh =
        reinterpret_cast<const uint8_t *>(&state.avx.regs[n]) + sseRegSize;
    memcpy(ymmh + n * ymmhSize, avxHigh, ymmhSize);
  }
}

static inline void user_to_state64(ds2::Architecture::X86_64::CPUState64 &state,
                                   struct xsave_struct const &xfpregs) {
  // Legacy (fxsave) registers
  state.x87.fctw = xfpregs.fpregs.fctw;
  state.x87.fstw = xfpregs.fpregs.fstw;
  state.x87.ftag = xfpregs.fpregs.ftag;
  state.x87.fop = xfpregs.fpregs.fop;
  state.x87.fioff = xfpregs.fpregs.fioff;
  state.x87.fiseg = xfpregs.fpregs.fiseg;
  state.x87.fooff = xfpregs.fpregs.fooff;
  state.x87.foseg = xfpregs.fpregs.foseg;

  auto st_space = reinterpret_cast<uint8_t const *>(xfpregs.fpregs.st_space);
  static const size_t x87DataSize = sizeof(state.x87.regs[0].data);
  static const size_t x87RegSize = x87DataSize + x87Padding;
  for (size_t n = 0; n < array_sizeof(state.x87.regs); n++) {
    memcpy(state.x87.regs[n].data, st_space + n * x87RegSize, x87DataSize);
  }

  // TODO: This hack is necessary because we don't handle EAVX registers.
  size_t numSSEState = array_sizeof(state.sse.regs);
  size_t numSSEUser =
      sizeof(xfpregs.fpregs.xmm_space) / sizeof(state.sse.regs[0]);
  size_t numSSE = std::min(numSSEState, numSSEUser);

  state.sse.mxcsr = xfpregs.fpregs.mxcsr;
  state.sse.mxcsrmask = xfpregs.fpregs.mxcsrmask;
  auto xmm_space = reinterpret_cast<uint8_t const *>(xfpregs.fpregs.xmm_space);
  static const size_t sseRegSize = sizeof(state.sse.regs[0]);
  for (size_t n = 0; n < numSSE; n++) {
    memcpy(&state.sse.regs[n], xmm_space + n * sseRegSize, sseRegSize);
  }

  state.xcr0 = xfpregs.fpregs.xcr0;

  // XSAVE Header
  state.xsave_header.xfeatures_mask = xfpregs.header.xfeatures_mask;

  //
  //  EAVX State
  //
  auto ymmh = reinterpret_cast<uint8_t const *>(xfpregs.ymmh);
  static const size_t avxSize = sizeof(state.avx.regs[0]);
  static const size_t ymmhSize = avxSize - sseRegSize;
  for (size_t n = 0; n < numSSE; n++) {
    auto avxHigh = reinterpret_cast<uint8_t *>(&state.avx.regs[n]) + sseRegSize;
    memcpy(avxHigh, ymmh + n * ymmhSize, ymmhSize);
  }
}

static inline void
state64_to_user(struct xsave_struct &xfpregs,
                ds2::Architecture::X86_64::CPUState64 const &state) {
  // Legacy (fxsave) registers
  xfpregs.fpregs.fctw = state.x87.fctw;
  xfpregs.fpregs.fstw = state.x87.fstw;
  xfpregs.fpregs.ftag = state.x87.ftag;
  xfpregs.fpregs.fop = state.x87.fop;
  xfpregs.fpregs.fiseg = state.x87.fiseg;
  xfpregs.fpregs.fioff = state.x87.fioff;
  xfpregs.fpregs.foseg = state.x87.foseg;
  xfpregs.fpregs.fooff = state.x87.fooff;

  auto st_space = reinterpret_cast<uint8_t *>(xfpregs.fpregs.st_space);
  static const size_t x87DataSize = sizeof(state.x87.regs[0].data);
  static const size_t x87RegSize = x87DataSize + x87Padding;
  for (size_t n = 0; n < array_sizeof(state.x87.regs); n++) {
    memcpy(st_space + n * x87RegSize, state.x87.regs[n].data, x87DataSize);
  }

  // This hack is necessary because we don't handle EAVX registers
  size_t numSSEState = array_sizeof(state.sse.regs);
  size_t numSSEUser =
      sizeof(xfpregs.fpregs.xmm_space) / sizeof(state.sse.regs[0]);
  size_t numSSE = std::min(numSSEState, numSSEUser);

  xfpregs.fpregs.mxcsr = state.sse.mxcsr;
  xfpregs.fpregs.mxcsrmask = state.sse.mxcsrmask;
  auto xmm_space = reinterpret_cast<uint8_t *>(xfpregs.fpregs.xmm_space);
  static const size_t sseRegSize = sizeof(state.sse.regs[0]);
  for (size_t n = 0; n < numSSE; n++) {
    memcpy(xmm_space + n * sseRegSize, &state.sse.regs[n], sseRegSize);
  }

  xfpregs.fpregs.xcr0 = state.xcr0;

  // XSAVE Header
  xfpregs.header.xfeatures_mask = state.xsave_header.xfeatures_mask;
  if (state.xcr0 & XFeature::X86_X87) {
    xfpregs.header.xfeatures_mask |= XFeature::X86_X87;
  }
  if (state.xcr0 & XFeature::X86_SSE) {
    xfpregs.header.xfeatures_mask |= XFeature::X86_SSE;
  }
  if (state.xcr0 & XFeature::X86_AVX) {
    xfpregs.header.xfeatures_mask |= XFeature::X86_AVX;
  }
  // TODO: Support xcomp_bv. See 13.4.2 XSAVE Header in Intels' 64 and IA32
  // Architecture Manual to understand the purpose of xcomp_bv.
  xfpregs.header.xcomp_bv = 0;

  //
  //  EAVX State
  //
  auto ymmh = reinterpret_cast<uint8_t *>(xfpregs.ymmh);
  static const size_t avxSize = sizeof(state.avx.regs[0]);
  static const size_t ymmhSize = avxSize - sseRegSize;
  for (size_t n = 0; n < numSSE; n++) {
    auto avxHigh =
        reinterpret_cast<const uint8_t *>(&state.avx.regs[n]) + sseRegSize;
    memcpy(ymmh + n * ymmhSize, avxHigh, ymmhSize);
  }
}

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

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
  struct xsave_struct xfpregs;
  struct iovec fpregs_iovec;
  fpregs_iovec.iov_base = &xfpregs;
  fpregs_iovec.iov_len = sizeof(xfpregs);

  // TODO: If this fails (i.e. ptrace returns < 0), it means XSAVE is most
  // likely unsupported and we can't expect XSAVE to work. In this case, we
  // should fallback to FXSAVE and only read the legacy portion of our xsave
  // state (x87, MMX, SSE).
  // If this call fails, don't return failure, since AVX may not be available
  // on this CPU.
  if (wrapPtrace(PTRACE_GETREGSET, pid, NT_X86_XSTATE, &fpregs_iovec) == 0) {
    if (pinfo.pointerSize == sizeof(uint32_t)) {
      user_to_state32(state.state32, xfpregs);
    } else {
      user_to_state64(state.state64, xfpregs);
    }
  }

  // Read the debug registers
  size_t debugRegOffset = offsetof(struct user, u_debugreg);
  size_t debugRegSize = sizeof(((struct user *)0)->u_debugreg[0]);
  for (size_t i = 0; i < array_sizeof(state.state64.dr.dr); ++i) {
    // dr4 and dr5 are reserved and not used
    if (i == 4 || i == 5) {
      continue;
    }

    errno = 0;
    long val = wrapPtrace(PTRACE_PEEKUSER, pid,
                          debugRegOffset + i * debugRegSize, nullptr);
    if (errno != 0) {
      return Platform::TranslateError();
    }

    if (state.is32) {
      state.state32.dr.dr[i] = val;
    } else {
      state.state64.dr.dr[i] = val;
    }
  }

  return kSuccess;
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &pinfo,
                                Architecture::CPUState const &state) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

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
  struct xsave_struct xfpregs;
  struct iovec fpregs_iovec;
  fpregs_iovec.iov_base = &xfpregs;
  fpregs_iovec.iov_len = sizeof(xfpregs);

  if (state.is32) {
    state32_to_user(xfpregs, state.state32);
  } else {
    state64_to_user(xfpregs, state.state64);
  }

  // TODO: If this fails (i.e. ptrace returns < 0), it means XSAVE is most
  // likely unsupported and we can't expect XSAVE to work. In this case, we
  // should fallback to FXSAVE and only write the legacy portion of our xsave
  // state (x87, MMX, SSE).
  wrapPtrace(PTRACE_SETREGSET, pid, NT_X86_XSTATE, &fpregs_iovec);

  // Write the debug registers
  size_t debugRegOffset = offsetof(struct user, u_debugreg);
  size_t debugRegSize = sizeof(((struct user *)0)->u_debugreg[0]);
  for (size_t i = 0; i < array_sizeof(state.state64.dr.dr); ++i) {
    // dr4 and dr5 are reserved and not used
    if (i == 4 || i == 5) {
      continue;
    }

    if (wrapPtrace(PTRACE_POKEUSER, pid, debugRegOffset + i * debugRegSize,
                   state.is32 ? state.state32.dr.dr[i]
                              : state.state64.dr.dr[i]) < 0) {
      return Platform::TranslateError();
    }
  }

  return kSuccess;
}
} // namespace Linux
} // namespace Host
} // namespace ds2
