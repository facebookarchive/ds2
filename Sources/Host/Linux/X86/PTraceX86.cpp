//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/Linux/PTrace.h"
#include "DebugServer2/Host/Linux/ExtraSyscalls.h"

#define super ds2::Host::POSIX::PTrace

#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <elf.h>

using ds2::Host::Linux::PTrace;
using ds2::ErrorCode;

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
}
}
}

void PTrace::initCPUState(ProcessId pid) {
  if (_privateData != nullptr)
    return;

  _privateData = new PTracePrivateData;
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
  struct user_regs_struct gprs;
  if (wrapPtrace(PTRACE_GETREGS, pid, nullptr, &gprs) < 0)
    return TranslateErrno();

  state.gp.eax = gprs.eax;
  state.gp.ecx = gprs.ecx;
  state.gp.edx = gprs.edx;
  state.gp.ebx = gprs.ebx;
  state.gp.esi = gprs.esi;
  state.gp.edi = gprs.edi;
  state.gp.ebp = gprs.ebp;
  state.gp.esp = gprs.esp;
  state.gp.eip = gprs.eip;
  state.gp.cs = gprs.xcs & 0xffff;
  state.gp.ss = gprs.xss & 0xffff;
  state.gp.ds = gprs.xds & 0xffff;
  state.gp.es = gprs.xes & 0xffff;
  state.gp.fs = gprs.xfs & 0xffff;
  state.gp.gs = gprs.xgs & 0xffff;
  state.gp.eflags = gprs.eflags;
  state.linux_gp.orig_eax = gprs.orig_eax;

  //
  // Read X87 and SSE state
  //
  struct user_fpxregs_struct fxrs;
  if (wrapPtrace(PTRACE_GETFPXREGS, pid, nullptr, &fxrs) == 0) {
    // X87 State
    state.x87.fstw = fxrs.swd;
    state.x87.fctw = fxrs.cwd;
    state.x87.ftag = fxrs.twd;
    state.x87.fop = fxrs.fop;
    state.x87.fiseg = fxrs.fcs;
    state.x87.fioff = fxrs.fip;
    state.x87.foseg = fxrs.fos;
    state.x87.fooff = fxrs.foo;

    uint8_t const *st_space = reinterpret_cast<uint8_t const *>(fxrs.st_space);
    for (size_t n = 0; n < 8; n++) {
      memcpy(state.x87.regs[n].bytes, st_space + n * 16,
             sizeof(state.x87.regs[n].bytes));
    }

    // SSE state
    state.sse.mxcsr = fxrs.mxcsr;
    state.sse.mxcsrmask = fxrs.reserved;
    uint8_t const *xmm_space =
        reinterpret_cast<uint8_t const *>(fxrs.xmm_space);
    for (size_t n = 0; n < 8; n++) {
      memcpy(&state.sse.regs[n], xmm_space + n * 16, sizeof(state.sse.regs[n]));
    }
  } else {
    //
    // Try reading only X87
    //
    struct user_fpregs_struct fprs;
    if (wrapPtrace(PTRACE_GETFPREGS, pid, nullptr, &fprs) == 0) {
      state.x87.fstw = fprs.swd;
      state.x87.fctw = fprs.cwd;
      state.x87.ftag = fprs.twd;
      state.x87.fiseg = fprs.fcs;
      state.x87.fioff = fprs.fip;
      state.x87.foseg = fprs.fos;
      state.x87.fooff = fprs.foo;

      uint8_t const *st_space =
          reinterpret_cast<uint8_t const *>(fprs.st_space);
      for (size_t n = 0; n < 8; n++) {
        memcpy(state.x87.regs[n].bytes, st_space + n * 10,
               sizeof(state.x87.regs[n].bytes));
      }
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
  struct user_regs_struct gprs;
  std::memset(&gprs, 0, sizeof(gprs));
  gprs.eax = state.gp.eax;
  gprs.ecx = state.gp.ecx;
  gprs.edx = state.gp.edx;
  gprs.ebx = state.gp.ebx;
  gprs.esi = state.gp.esi;
  gprs.edi = state.gp.edi;
  gprs.ebp = state.gp.ebp;
  gprs.esp = state.gp.esp;
  gprs.eip = state.gp.eip;
  gprs.xcs = state.gp.cs & 0xffff;
  gprs.xss = state.gp.ss & 0xffff;
  gprs.xds = state.gp.ds & 0xffff;
  gprs.xes = state.gp.es & 0xffff;
  gprs.xfs = state.gp.fs & 0xffff;
  gprs.xgs = state.gp.gs & 0xffff;
  gprs.eflags = state.gp.eflags;
  gprs.orig_eax = state.linux_gp.orig_eax;

  if (wrapPtrace(PTRACE_SETREGS, pid, nullptr, &gprs) < 0)
    return TranslateErrno();

  //
  // Write X87 and SSE state
  //
  struct user_fpxregs_struct fxrs;

  // X87 State
  fxrs.swd = state.x87.fstw;
  fxrs.cwd = state.x87.fctw;
  fxrs.twd = state.x87.ftag;
  fxrs.fop = state.x87.fop;
  fxrs.fcs = state.x87.fiseg;
  fxrs.fip = state.x87.fioff;
  fxrs.fos = state.x87.foseg;
  fxrs.foo = state.x87.fooff;

  uint8_t *st_space = reinterpret_cast<uint8_t *>(fxrs.st_space);
  for (size_t n = 0; n < 8; n++) {
    memcpy(st_space + n * 16, state.x87.regs[n].bytes,
           sizeof(state.x87.regs[n].bytes));
  }

  // SSE state
  fxrs.mxcsr = state.sse.mxcsr;
  fxrs.reserved = state.sse.mxcsrmask;
  uint8_t *xmm_space = reinterpret_cast<uint8_t *>(fxrs.xmm_space);
  for (size_t n = 0; n < 8; n++) {
    memcpy(xmm_space + n * 16, &state.sse.regs[n], sizeof(state.sse.regs[n]));
  }

  if (wrapPtrace(PTRACE_SETFPXREGS, pid, nullptr, &fxrs) < 0) {
    //
    // Try writing only X87
    //
    struct user_fpregs_struct fprs;
    fprs.swd = state.x87.fstw;
    fprs.cwd = state.x87.fctw;
    fprs.twd = state.x87.ftag;
    fprs.fcs = state.x87.fiseg;
    fprs.fip = state.x87.fioff;
    fprs.fos = state.x87.foseg;
    fprs.foo = state.x87.fooff;

    uint8_t *st_space = reinterpret_cast<uint8_t *>(fprs.st_space);
    for (size_t n = 0; n < 8; n++) {
      memcpy(st_space + n * 10, state.x87.regs[n].bytes,
             sizeof(state.x87.regs[n].bytes));
    }
    wrapPtrace(PTRACE_SETFPREGS, pid, nullptr, &fprs);
  }

  return kSuccess;
}
