//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#define __DS2_LOG_CLASS_NAME__ "Target::Thread"

#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Host/Windows/ExtraWrappers.h"
#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Utils/Log.h"

#include <windows.h>

using ds2::Host::Platform;

namespace ds2 {
namespace Target {
namespace Windows {

ErrorCode Thread::step(int signal, Address const &address) {
  DS2BUG("not implemented");
}

ErrorCode Thread::readCPUState(Architecture::CPUState &state) {
  CONTEXT context;

  memset(&context, 0, sizeof(context));
  context.ContextFlags = CONTEXT_INTEGER |        // GP registers.
                         CONTEXT_CONTROL |        // Some more GP + CPSR.
                         CONTEXT_FLOATING_POINT | // FP registers.
                         CONTEXT_DEBUG_REGISTERS; // Debug registers.

  BOOL result = GetThreadContext(_handle, &context);
  if (!result) {
    return Platform::TranslateError();
  }

  // GP registers + CPSR.
  state.gp.r0 = context.R0;
  state.gp.r1 = context.R1;
  state.gp.r2 = context.R2;
  state.gp.r3 = context.R3;
  state.gp.r4 = context.R4;
  state.gp.r5 = context.R5;
  state.gp.r6 = context.R6;
  state.gp.r7 = context.R7;
  state.gp.r8 = context.R8;
  state.gp.r9 = context.R9;
  state.gp.r10 = context.R10;
  state.gp.r11 = context.R11;
  state.gp.ip = context.R12;
  state.gp.sp = context.Sp;
  state.gp.lr = context.Lr;
  state.gp.pc = context.Pc;
  state.gp.cpsr = context.Cpsr;

  // TODO(sas): Handle floating point and debug registers.

  return kSuccess;
}

ErrorCode Thread::writeCPUState(Architecture::CPUState const &state) {
  CONTEXT context;

  memset(&context, 0, sizeof(context));
  // TODO(sas): Handle floats and debug registers.
  context.ContextFlags = CONTEXT_INTEGER | // GP registers.
                         CONTEXT_CONTROL;  // Some more GP + CPSR.

  // GP registers + CPSR.
  context.R0 = state.gp.r0;
  context.R1 = state.gp.r1;
  context.R2 = state.gp.r2;
  context.R3 = state.gp.r3;
  context.R4 = state.gp.r4;
  context.R5 = state.gp.r5;
  context.R6 = state.gp.r6;
  context.R7 = state.gp.r7;
  context.R8 = state.gp.r8;
  context.R9 = state.gp.r9;
  context.R10 = state.gp.r10;
  context.R11 = state.gp.r11;
  context.R12 = state.gp.ip;
  context.Sp = state.gp.sp;
  context.Lr = state.gp.lr;
  context.Pc = state.gp.pc;
  context.Cpsr = state.gp.cpsr;

  BOOL result = SetThreadContext(_handle, &context);
  if (!result) {
    return Platform::TranslateError();
  }

  return kSuccess;
}
}
}
}
