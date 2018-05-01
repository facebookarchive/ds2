//
// Copyright (c) 2014-present, Facebook, Inc.
// Copyright (c) 2015, Jakub Klama <jakub@ixsystems.com>
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/FreeBSD/PTrace.h"
#include "DebugServer2/Architecture/X86/RegisterCopy.h"
#include "DebugServer2/Host/Platform.h"

#include <elf.h>
#include <machine/fpu.h>
#include <machine/reg.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/user.h>

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace FreeBSD {

//
// 32-bits helpers
//

static inline void user_to_state32(ds2::Architecture::X86_64::CPUState32 &state,
                                   struct fpreg const &user) {
  //
  // X87 State
  //
  struct env87 *x87 = (struct env87 *)&user;
  state.x87.fstw = x87->en_sw;
  state.x87.fctw = x87->en_cw;
  state.x87.ftag = x87->en_tw;
  state.x87.fioff = x87->en_fip;
  state.x87.fiseg = x87->en_fcs;
  state.x87.fop = x87->en_opcode;
  state.x87.fooff = x87->en_foo;
  state.x87.foseg = x87->en_fos;

  uint8_t const *st_space = reinterpret_cast<uint8_t const *>(user.fpr_acc);
  for (size_t n = 0; n < 8; n++) {
    memcpy(state.x87.regs[n].data, st_space + n * 10,
           sizeof(state.x87.regs[n].data));
  }

  //
  // SSE State
  //
  struct envxmm *xmm = (struct envxmm *)&user.fpr_env;
  state.sse.mxcsr = xmm->en_mxcsr;
  state.sse.mxcsrmask = xmm->en_mxcsr_mask;

  uint8_t const *xmm_space = reinterpret_cast<uint8_t const *>(user.fpr_xacc);
  for (size_t n = 0; n < 8; n++) {
    memcpy(&state.sse.regs[n], xmm_space + n * 16, sizeof(state.sse.regs[n]));
  }
}

static inline void
state32_to_user(struct fpreg &user,
                ds2::Architecture::X86_64::CPUState32 const &state) {
  //
  // X87 State
  //
  struct env87 *x87 = (struct env87 *)&user.fpr_env;
  x87->en_sw = state.x87.fstw;
  x87->en_cw = state.x87.fctw;
  x87->en_tw = state.x87.ftag;
  x87->en_fip = state.x87.fioff;
  x87->en_fcs = state.x87.fiseg;
  x87->en_opcode = state.x87.fop;
  x87->en_foo = state.x87.fooff;
  x87->en_fos = state.x87.foseg;

  uint8_t *st_space = reinterpret_cast<uint8_t *>(user.fpr_acc);
  for (size_t n = 0; n < 8; n++) {
    memcpy(st_space + n * 10, state.x87.regs[n].data,
           sizeof(state.x87.regs[n].data));
  }

  //
  // SSE State
  //
  struct envxmm *xmm = (struct envxmm *)&user.fpr_env;
  xmm->en_mxcsr = state.sse.mxcsr;
  xmm->en_mxcsr_mask = state.sse.mxcsrmask;

  uint8_t *xmm_space = reinterpret_cast<uint8_t *>(user.fpr_xacc);
  for (size_t n = 0; n < 8; n++) {
    memcpy(xmm_space + n * 16, &state.sse.regs[n], sizeof(state.sse.regs[n]));
  }
}

//
// 64-bit helpers
//

static inline void user_to_state64(ds2::Architecture::X86_64::CPUState64 &state,
                                   struct fpreg const &user) {
  //
  // X87 State
  //
  struct env87 *x87 = (struct env87 *)&user;
  state.x87.fstw = x87->en_sw;
  state.x87.fctw = x87->en_cw;
  state.x87.ftag = x87->en_tw;

  // state.x87.fop = user.r_fop;
  // state.x87.firip = user.r_rip;
  // state.x87.forip = user.r_rdp;

  uint8_t const *st_space = reinterpret_cast<uint8_t const *>(user.fpr_acc);
  for (size_t n = 0; n < 8; n++) {
    memcpy(state.x87.regs[n].data, st_space + n * 10,
           sizeof(state.x87.regs[n].data));
  }

  //
  // SSE State
  //
  struct envxmm *xmm = (struct envxmm *)&user.fpr_env;
  state.sse.mxcsr = xmm->en_mxcsr;
  state.sse.mxcsrmask = xmm->en_mxcsr_mask;
  uint8_t const *xmm_space = reinterpret_cast<uint8_t const *>(user.fpr_xacc);
  for (size_t n = 0; n < 16; n++) {
    memcpy(&state.sse.regs[n], xmm_space + n * 16, sizeof(state.sse.regs[n]));
  }
}

static inline void
state64_to_user(struct fpreg &user,
                ds2::Architecture::X86_64::CPUState64 const &state) {
  //
  // X87 State
  //
  struct env87 *x87 = (struct env87 *)&user;
  x87->en_sw = state.x87.fstw;
  x87->en_cw = state.x87.fctw;
  x87->en_tw = state.x87.ftag;
  // user.r_fop = state.x87.fop;
  // user.r_rip = state.x87.firip;
  // user.r_rdp = state.x87.forip;

  uint8_t *st_space = reinterpret_cast<uint8_t *>(user.fpr_acc);
  for (size_t n = 0; n < 8; n++) {
    memcpy(st_space + n * 16, state.x87.regs[n].data,
           sizeof(state.x87.regs[n].data));
  }

  //
  // SSE State
  //
  struct envxmm *xmm = (struct envxmm *)&user.fpr_env;
  xmm->en_mxcsr = state.sse.mxcsr;
  xmm->en_mxcsr_mask = state.sse.mxcsrmask;
  uint8_t *xmm_space = reinterpret_cast<uint8_t *>(user.fpr_xacc);
  for (size_t n = 0; n < 16; n++) {
    memcpy(xmm_space + n * 16, &state.sse.regs[n], sizeof(state.sse.regs[n]));
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
  struct reg gprs;
  if (wrapPtrace(PT_GETREGS, pid, &gprs, nullptr) < 0)
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
  struct fpreg fprs;
  if (wrapPtrace(PT_GETFPREGS, pid, &fprs, nullptr) == 0) {
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
  CHK(ptidToPid(ptid, pid));

  if (pinfo.pointerSize == sizeof(uint32_t) && !state.is32)
    return kErrorInvalidArgument;
  else if (pinfo.pointerSize != sizeof(uint32_t) && state.is32)
    return kErrorInvalidArgument;

  //
  // Write GPRs
  //
  struct reg gprs;
  if (state.is32) {
    Architecture::X86::state32_to_user(gprs, state.state32);
  } else {
    Architecture::X86::state64_to_user(gprs, state.state64);
  }

  if (wrapPtrace(PT_SETREGS, pid, &gprs, nullptr) < 0)
    return Platform::TranslateError();

  //
  // Write X87 and SSE state
  //
  struct fpreg fprs;
  if (state.is32) {
    state32_to_user(fprs, state.state32);
  } else {
    state64_to_user(fprs, state.state64);
  }

  wrapPtrace(PT_SETFPREGS, pid, &fprs, nullptr);

  return kSuccess;
}
} // namespace FreeBSD
} // namespace Host
} // namespace ds2
