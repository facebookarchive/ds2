//
// Copyright (c) 2014-present, Facebook, Inc.
// Copyright (c) 2015, Corentin Derbois <cderbois@gmail.com>
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/Darwin/Mach.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach/thread_info.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/user.h>

#define super ds2::Host::POSIX::Mach

namespace ds2 {
namespace Host {
namespace Darwin {

ErrorCode Mach::readCPUState(ProcessThreadId const &ptid,
                             ProcessInfo const &pinfo,
                             Architecture::CPUState &ds2_state) {
  pid_t tid;

  if (!ptid.valid())
    return kErrorInvalidArgument;

  if (!(ptid.tid <= kAnyThreadId)) {
    tid = ptid.tid;
  } else {
    tid = ptid.pid;
  }

  thread_t thread = getMachThread(ptid.pid, tid);
  if (thread == THREAD_NULL)
    return kErrorProcessNotFound;

  kern_return_t kret;
  mach_msg_type_number_t stateCount = x86_THREAD_STATE_COUNT;
  x86_thread_state_t state;

  kret = thread_get_state(thread, x86_THREAD_STATE, (thread_state_t)&state,
                          &stateCount);
  if (kret != KERN_SUCCESS) {
    DS2LOG(Error, "Fail to get the reg of the pid: %s",
           mach_error_string(kret));
    return kErrorInvalidArgument;
  }

  ds2_state.is32 = false;
  ds2_state.state64.gp.rax = state.uts.ts64.__rax;
  ds2_state.state64.gp.rcx = state.uts.ts64.__rcx;
  ds2_state.state64.gp.rdx = state.uts.ts64.__rdx;
  ds2_state.state64.gp.rbx = state.uts.ts64.__rbx;
  ds2_state.state64.gp.rsi = state.uts.ts64.__rsi;
  ds2_state.state64.gp.rdi = state.uts.ts64.__rdi;
  ds2_state.state64.gp.rbp = state.uts.ts64.__rbp;
  ds2_state.state64.gp.rsp = state.uts.ts64.__rsp;
  ds2_state.state64.gp.r8 = state.uts.ts64.__r8;
  ds2_state.state64.gp.r9 = state.uts.ts64.__r9;
  ds2_state.state64.gp.r10 = state.uts.ts64.__r10;
  ds2_state.state64.gp.r11 = state.uts.ts64.__r11;
  ds2_state.state64.gp.r12 = state.uts.ts64.__r12;
  ds2_state.state64.gp.r13 = state.uts.ts64.__r13;
  ds2_state.state64.gp.r14 = state.uts.ts64.__r14;
  ds2_state.state64.gp.r15 = state.uts.ts64.__r15;
  ds2_state.state64.gp.rip = state.uts.ts64.__rip;
  ds2_state.state64.gp.cs = state.uts.ts64.__cs & 0xffff;
  ds2_state.state64.gp.fs = state.uts.ts64.__fs & 0xffff;
  ds2_state.state64.gp.gs = state.uts.ts64.__gs & 0xffff;
  ds2_state.state64.gp.eflags = state.uts.ts64.__rflags;

  /*
   * TODO: Darwin register struct doesn't have ss/ds/es ?
   */

  ds2_state.state64.gp.ss = 0;
  ds2_state.state64.gp.ds = 0;
  ds2_state.state64.gp.es = 0;

  return kSuccess;
}

ErrorCode Mach::writeCPUState(ProcessThreadId const &ptid,
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
  DS2BUG("not implemented");

  return kSuccess;
}
}
}
}
