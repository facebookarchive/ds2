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

namespace ds2 {
namespace Host {
namespace Darwin {

ErrorCode Mach::readCPUState(ProcessThreadId const &ptid,
                             ProcessInfo const &pinfo,
                             Architecture::CPUState &state) {
  if (!ptid.valid()) {
    return kErrorInvalidArgument;
  }

  thread_t thread = getMachThread(ptid);
  if (thread == THREAD_NULL) {
    return kErrorProcessNotFound;
  }

  mach_msg_type_number_t stateCount = x86_THREAD_STATE_COUNT;
  x86_thread_state_t threadState;
  kern_return_t kret = thread_get_state(
      thread, x86_THREAD_STATE, (thread_state_t)&threadState, &stateCount);
  if (kret != KERN_SUCCESS) {
    return kErrorInvalidArgument;
  }

  state.is32 = false;
  state.state64.gp.rax = threadState.uts.ts64.__rax;
  state.state64.gp.rcx = threadState.uts.ts64.__rcx;
  state.state64.gp.rdx = threadState.uts.ts64.__rdx;
  state.state64.gp.rbx = threadState.uts.ts64.__rbx;
  state.state64.gp.rsi = threadState.uts.ts64.__rsi;
  state.state64.gp.rdi = threadState.uts.ts64.__rdi;
  state.state64.gp.rbp = threadState.uts.ts64.__rbp;
  state.state64.gp.rsp = threadState.uts.ts64.__rsp;
  state.state64.gp.r8 = threadState.uts.ts64.__r8;
  state.state64.gp.r9 = threadState.uts.ts64.__r9;
  state.state64.gp.r10 = threadState.uts.ts64.__r10;
  state.state64.gp.r11 = threadState.uts.ts64.__r11;
  state.state64.gp.r12 = threadState.uts.ts64.__r12;
  state.state64.gp.r13 = threadState.uts.ts64.__r13;
  state.state64.gp.r14 = threadState.uts.ts64.__r14;
  state.state64.gp.r15 = threadState.uts.ts64.__r15;
  state.state64.gp.rip = threadState.uts.ts64.__rip;
  state.state64.gp.cs = threadState.uts.ts64.__cs & 0xffff;
  state.state64.gp.fs = threadState.uts.ts64.__fs & 0xffff;
  state.state64.gp.gs = threadState.uts.ts64.__gs & 0xffff;
  state.state64.gp.eflags = threadState.uts.ts64.__rflags;

  // TODO: Darwin register struct doesn't have ss/ds/es ?
  state.state64.gp.ss = 0;
  state.state64.gp.ds = 0;
  state.state64.gp.es = 0;

  return kSuccess;
}

ErrorCode Mach::writeCPUState(ProcessThreadId const &ptid,
                              ProcessInfo const &pinfo,
                              Architecture::CPUState const &state) {
  if (!ptid.valid()) {
    return kErrorInvalidArgument;
  }

  thread_t thread = getMachThread(ptid);
  if (thread == THREAD_NULL) {
    return kErrorProcessNotFound;
  }

  x86_thread_state_t threadState;

  threadState.tsh.flavor = x86_THREAD_STATE64;
  threadState.tsh.count = x86_THREAD_STATE64_COUNT;

  threadState.uts.ts64.__rax = state.state64.gp.rax;
  threadState.uts.ts64.__rcx = state.state64.gp.rcx;
  threadState.uts.ts64.__rdx = state.state64.gp.rdx;
  threadState.uts.ts64.__rbx = state.state64.gp.rbx;
  threadState.uts.ts64.__rsi = state.state64.gp.rsi;
  threadState.uts.ts64.__rdi = state.state64.gp.rdi;
  threadState.uts.ts64.__rbp = state.state64.gp.rbp;
  threadState.uts.ts64.__rsp = state.state64.gp.rsp;
  threadState.uts.ts64.__r8 = state.state64.gp.r8;
  threadState.uts.ts64.__r9 = state.state64.gp.r9;
  threadState.uts.ts64.__r10 = state.state64.gp.r10;
  threadState.uts.ts64.__r11 = state.state64.gp.r11;
  threadState.uts.ts64.__r12 = state.state64.gp.r12;
  threadState.uts.ts64.__r13 = state.state64.gp.r13;
  threadState.uts.ts64.__r14 = state.state64.gp.r14;
  threadState.uts.ts64.__r15 = state.state64.gp.r15;
  threadState.uts.ts64.__rip = state.state64.gp.rip;
  threadState.uts.ts64.__cs = state.state64.gp.cs & 0xffff;
  threadState.uts.ts64.__fs = state.state64.gp.fs & 0xffff;
  threadState.uts.ts64.__gs = state.state64.gp.gs & 0xffff;
  threadState.uts.ts64.__rflags = state.state64.gp.eflags;

  mach_msg_type_number_t threadStateCount = x86_THREAD_STATE_COUNT;
  kern_return_t kret = thread_set_state(
      thread, x86_THREAD_STATE, (thread_state_t)&threadState, threadStateCount);
  if (kret != KERN_SUCCESS) {
    return kErrorInvalidArgument;
  }

  return kSuccess;
}
} // namespace Darwin
} // namespace Host
} // namespace ds2
