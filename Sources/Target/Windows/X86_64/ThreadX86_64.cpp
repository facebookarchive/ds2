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
#include "DebugServer2/Architecture/X86/RegisterCopy.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Host/Windows/ExtraWrappers.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Utils/Log.h"

#include <windows.h>

using ds2::Host::Platform;

namespace ds2 {
namespace Target {
namespace Windows {

ErrorCode Thread::step(int signal, Address const &address) {
  // Windows doesn't have a dedicated single-step call. We have to set the
  // single step flag (TF, 8th bit) in eflags and resume the thread.
  CHK(modifyRegisters([](Architecture::CPUState &state) {
    state.state64.gp.eflags |= (1 << 8);
  }));
  CHK(resume(signal, address));

  return kSuccess;
}

ErrorCode Thread::readCPUState(Architecture::CPUState &state) {
  // TODO(sas): Handle floats, SSE, AVX and debug registers.
  DWORD flags = CONTEXT_INTEGER |        // GP registers.
                CONTEXT_CONTROL |        // Some more GP + cs/ss.
                CONTEXT_SEGMENTS |       // Data segment selectors.
                CONTEXT_DEBUG_REGISTERS; // Debug registers.

  ProcessInfo pinfo;
  CHK(process()->getInfo(pinfo));

  DS2ASSERT(pinfo.pointerSize == sizeof(uint64_t));

  // TODO(sas): Support WOW64.
  if (pinfo.pointerSize == sizeof(uint64_t)) {
    CONTEXT context;

    std::memset(&context, 0, sizeof(context));
    context.ContextFlags = flags;

    BOOL result = GetThreadContext(_handle, &context);
    if (!result) {
      return Platform::TranslateError();
    }

    state.is32 = false;
    Architecture::X86::user_to_state64(state.state64, context);

    // Debug registers
    state.state64.dr.dr[0] = context.Dr0;
    state.state64.dr.dr[1] = context.Dr1;
    state.state64.dr.dr[2] = context.Dr2;
    state.state64.dr.dr[3] = context.Dr3;
    state.state64.dr.dr[6] = context.Dr6;
    state.state64.dr.dr[7] = context.Dr7;
  }

  return kSuccess;
}

ErrorCode Thread::writeCPUState(Architecture::CPUState const &state) {
  // TODO(sas): Handle floats, SSE, AVX and debug registers.
  DWORD flags = CONTEXT_INTEGER |        // GP registers.
                CONTEXT_CONTROL |        // Some more GP + cs/ss.
                CONTEXT_SEGMENTS |       // Data segment selectors.
                CONTEXT_DEBUG_REGISTERS; // Debug registers.

  ProcessInfo pinfo;
  CHK(process()->getInfo(pinfo));

  DS2ASSERT(pinfo.pointerSize == sizeof(uint64_t));

  // TODO(sas): Support WOW64.
  if (pinfo.pointerSize == sizeof(uint64_t)) {
    CONTEXT context;

    std::memset(&context, 0, sizeof(context));
    context.ContextFlags = flags;
    Architecture::X86::state64_to_user(context, state.state64);

    // Debug registers
    context.Dr0 = state.state64.dr.dr[0];
    context.Dr1 = state.state64.dr.dr[1];
    context.Dr2 = state.state64.dr.dr[2];
    context.Dr3 = state.state64.dr.dr[3];
    context.Dr6 = state.state64.dr.dr[6];
    context.Dr7 = state.state64.dr.dr[7];

    BOOL result = SetThreadContext(_handle, &context);
    if (!result) {
      return Platform::TranslateError();
    }
  }

  return kSuccess;
}
} // namespace Windows
} // namespace Target
} // namespace ds2
