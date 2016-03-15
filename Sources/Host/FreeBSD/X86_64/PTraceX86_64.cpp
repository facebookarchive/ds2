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
#include "DebugServer2/Host/Platform.h"

#include <DebugServer2/Architecture/X86/CPUState.h>
#include <DebugServer2/Architecture/X86_64/CPUState.h>
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

//
// 32-bits helpers
//

static inline void user_to_state32(ds2::Architecture::X86_64::CPUState32 &state,
                                   struct reg const &user) {
  state.gp.eax = user.r_rax;
  state.gp.ecx = user.r_rcx;
  state.gp.edx = user.r_rdx;
  state.gp.ebx = user.r_rbx;
  state.gp.esi = user.r_rsi;
  state.gp.edi = user.r_rdi;
  state.gp.ebp = user.r_rbp;
  state.gp.esp = user.r_rsp;
  state.gp.eip = user.r_rip;
  state.gp.cs = user.r_cs & 0xffff;
  state.gp.ss = user.r_ss & 0xffff;
  state.gp.ds = user.r_ds & 0xffff;
  state.gp.es = user.r_es & 0xffff;
  state.gp.fs = user.r_fs & 0xffff;
  state.gp.gs = user.r_gs & 0xffff;
  state.gp.eflags = user.r_rflags;
}

static inline void
state32_to_user(struct reg &user,
                ds2::Architecture::X86_64::CPUState32 const &state) {
  user.r_rax = state.gp.eax;
  user.r_rcx = state.gp.ecx;
  user.r_rdx = state.gp.edx;
  user.r_rbx = state.gp.ebx;
  user.r_rsi = state.gp.esi;
  user.r_rdi = state.gp.edi;
  user.r_rbp = state.gp.ebp;
  user.r_rsp = state.gp.esp;
  user.r_rip = state.gp.eip;
  user.r_cs = state.gp.cs;
  user.r_ss = state.gp.ss;
  user.r_ds = state.gp.ds;
  user.r_es = state.gp.es;
  user.r_fs = state.gp.fs;
  user.r_gs = state.gp.gs;
  user.r_rflags = state.gp.eflags;
}

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
    memcpy(state.x87.regs[n].bytes, st_space + n * 10,
           sizeof(state.x87.regs[n].bytes));
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
    memcpy(st_space + n * 10, state.x87.regs[n].bytes,
           sizeof(state.x87.regs[n].bytes));
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
                                   struct reg const &user) {
  state.gp.rax = user.r_rax;
  state.gp.rcx = user.r_rcx;
  state.gp.rdx = user.r_rdx;
  state.gp.rbx = user.r_rbx;
  state.gp.rsi = user.r_rsi;
  state.gp.rdi = user.r_rdi;
  state.gp.rbp = user.r_rbp;
  state.gp.rsp = user.r_rsp;
  state.gp.r8 = user.r_r8;
  state.gp.r9 = user.r_r9;
  state.gp.r10 = user.r_r10;
  state.gp.r11 = user.r_r11;
  state.gp.r12 = user.r_r12;
  state.gp.r13 = user.r_r13;
  state.gp.r14 = user.r_r14;
  state.gp.r15 = user.r_r15;
  state.gp.rip = user.r_rip;
  state.gp.cs = user.r_cs & 0xffff;
  state.gp.ss = user.r_ss & 0xffff;
  state.gp.ds = user.r_ds & 0xffff;
  state.gp.es = user.r_es & 0xffff;
  state.gp.fs = user.r_fs & 0xffff;
  state.gp.gs = user.r_gs & 0xffff;
  state.gp.eflags = user.r_rflags;
}

static inline void
state64_to_user(struct reg &user,
                ds2::Architecture::X86_64::CPUState64 const &state) {
  user.r_rax = state.gp.rax;
  user.r_rcx = state.gp.rcx;
  user.r_rdx = state.gp.rdx;
  user.r_rbx = state.gp.rbx;
  user.r_rsi = state.gp.rsi;
  user.r_rdi = state.gp.rdi;
  user.r_rbp = state.gp.rbp;
  user.r_rsp = state.gp.rsp;
  user.r_r8 = state.gp.r8;
  user.r_r9 = state.gp.r9;
  user.r_r10 = state.gp.r10;
  user.r_r11 = state.gp.r11;
  user.r_r12 = state.gp.r12;
  user.r_r13 = state.gp.r13;
  user.r_r14 = state.gp.r14;
  user.r_r15 = state.gp.r15;
  user.r_rip = state.gp.rip;
  user.r_cs = state.gp.cs;
  user.r_ss = state.gp.ss;
  user.r_ds = state.gp.ds;
  user.r_es = state.gp.es;
  user.r_fs = state.gp.fs;
  user.r_gs = state.gp.gs;
  user.r_rflags = state.gp.eflags;
}

static inline void user_to_state64(ds2::Architecture::X86_64::CPUState64 &state,
                                   struct fpreg const &user) {
  //
  // X87 State
  //
  struct env87 *x87 = (struct env87 *)&user;
  state.x87.fstw = x87->en_sw;
  state.x87.fctw = x87->en_cw;
  state.x87.ftag = x87->en_tw;
  state.x87.firip = x87->en_fip;

  // state.x87.fop = user.r_fop;
  // state.x87.firip = user.r_rip;
  // state.x87.forip = user.r_rdp;

  uint8_t const *st_space = reinterpret_cast<uint8_t const *>(user.fpr_acc);
  for (size_t n = 0; n < 8; n++) {
    memcpy(state.x87.regs[n].bytes, st_space + n * 10,
           sizeof(state.x87.regs[n].bytes));
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
    memcpy(st_space + n * 16, state.x87.regs[n].bytes,
           sizeof(state.x87.regs[n].bytes));
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
  struct reg gprs;
  if (wrapPtrace(PT_GETREGS, pid, &gprs, nullptr) < 0)
    return Platform::TranslateError();

  if (pinfo.pointerSize == sizeof(uint32_t)) {
    state.is32 = true;
    user_to_state32(state.state32, gprs);
  } else {
    state.is32 = false;
    user_to_state64(state.state64, gprs);
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

  if (pinfo.pointerSize == sizeof(uint32_t) && !state.is32)
    return kErrorInvalidArgument;
  else if (pinfo.pointerSize != sizeof(uint32_t) && state.is32)
    return kErrorInvalidArgument;

  //
  // Write GPRs
  //
  struct reg gprs;
  if (state.is32) {
    state32_to_user(gprs, state.state32);
  } else {
    state64_to_user(gprs, state.state64);
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
}
}
}
