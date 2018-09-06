//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Architecture/ARM/SoftwareSingleStep.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Host/Windows/ExtraWrappers.h"
#include "DebugServer2/Utils/Log.h"

#include <windows.h>

using ds2::Host::Platform;

namespace ds2 {
namespace Target {
namespace Windows {

ErrorCode Thread::step(int signal, Address const &address) {
  if (_state == kInvalid || _state == kRunning) {
    return kErrorInvalidArgument;
  } else if (_state == kTerminated) {
    return kErrorProcessNotFound;
  }

  DS2LOG(Debug, "stepping tid %d", tid());

  // Prepare a software (arch-dependent) single step and resume execution.
  Architecture::CPUState state;
  CHK(readCPUState(state));
  CHK(PrepareSoftwareSingleStep(
      process(), process()->softwareBreakpointManager(), state, address));
  CHK(resume(signal, address));

  return kSuccess;
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

  // Floating point registers.
  static_assert(sizeof(context.D) == sizeof(state.vfp.dbl),
                "floating point register count mismatch");
  for (size_t i = 0; i < array_sizeof(context.D); ++i) {
    state.vfp.dbl[i].value = context.D[i];
  }
  state.vfp.fpscr = context.Fpscr;

  if (state.isThumb()) {
    if (state.gp.pc & 1ULL) {
      DS2LOG(Debug, "removing thumb bit from pc and lr");
      state.gp.pc &= ~1ULL;
    } else {
      DS2LOG(Warning,
             "CPU is in thumb mode but doesn't have thumb bit set in pc");
    }
  }

  // TODO(sas): Handle debug registers.

  return kSuccess;
}

ErrorCode Thread::writeCPUState(Architecture::CPUState const &state) {
  CONTEXT context;

  memset(&context, 0, sizeof(context));
  // TODO(sas): Handle debug registers.
  context.ContextFlags = CONTEXT_INTEGER |       // GP registers.
                         CONTEXT_CONTROL |       // Some more GP + CPSR.
                         CONTEXT_FLOATING_POINT; // FP registers.

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

  // Floating point registers.
  for (size_t i = 0; i < array_sizeof(context.D); ++i) {
    context.D[i] = state.vfp.dbl[i].value;
  }
  context.Fpscr = state.vfp.fpscr;

  if (state.isThumb()) {
    DS2ASSERT(!(state.gp.pc & 1ULL));
    DS2LOG(Debug, "setting back thumb bit on pc and lr");
    context.Pc |= 1ULL;
  }

  BOOL result = SetThreadContext(_handle, &context);
  if (!result) {
    return Platform::TranslateError();
  }

  return kSuccess;
}
} // namespace Windows
} // namespace Target
} // namespace ds2
