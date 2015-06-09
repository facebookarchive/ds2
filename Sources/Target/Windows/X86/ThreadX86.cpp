//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#define __DS2_LOG_CLASS_NAME__ "Target::Thread"

#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Utils/Log.h"

namespace ds2 {
namespace Target {
namespace Windows {

ErrorCode Thread::readCPUState(Architecture::CPUState &state) {
  CONTEXT context;

  memset(&context, 0, sizeof context);
  context.ContextFlags = CONTEXT_INTEGER |           // GP registers.
                         CONTEXT_CONTROL |           // Some more GP + cs/ss.
                         CONTEXT_SEGMENTS |          // Data segment selectors.
                         CONTEXT_FLOATING_POINT |    // FP registers.
                         CONTEXT_EXTENDED_REGISTERS; // SSE registers.

  BOOL result = GetThreadContext(_handle, &context);
  if (!result) {
    DS2LOG(Target, Error, "Unable to GetThreadContext: %d, handle=%p",
           GetLastError(), _handle);
    return kErrorUnknown;
  }

  // GP registers + segment selectors.
  state.gp.eax = context.Eax;
  state.gp.ecx = context.Ecx;
  state.gp.edx = context.Edx;
  state.gp.ebx = context.Ebx;
  state.gp.esi = context.Esi;
  state.gp.edi = context.Edi;
  state.gp.ebp = context.Ebp;
  state.gp.esp = context.Esp;
  state.gp.eip = context.Eip;
  state.gp.cs = context.SegCs & 0xffff;
  state.gp.ss = context.SegSs & 0xffff;
  state.gp.ds = context.SegDs & 0xffff;
  state.gp.es = context.SegEs & 0xffff;
  state.gp.fs = context.SegFs & 0xffff;
  state.gp.gs = context.SegGs & 0xffff;
  state.gp.eflags = context.EFlags;

  // x87 state
  state.x87.fstw = context.FloatSave.StatusWord;
  state.x87.fctw = context.FloatSave.ControlWord;
  state.x87.ftag = context.FloatSave.TagWord;
  state.x87.fiseg = context.FloatSave.ErrorSelector;
  state.x87.fioff = context.FloatSave.ErrorOffset;
  state.x87.foseg = context.FloatSave.DataSelector;
  state.x87.fooff = context.FloatSave.DataOffset;
  // TODO(sas): Figure out where this is stored.
  // state.x87.fop = ???;

  uint8_t const *st_space =
      reinterpret_cast<uint8_t const *>(context.FloatSave.RegisterArea);
  for (size_t n = 0; n < 8; n++) {
    static const int reg_size = sizeof(state.x87.regs[0].bytes);
    memcpy(state.x87.regs[n].bytes, st_space + n * reg_size, reg_size);
  }

  // SSE state
  if (IsProcessorFeaturePresent(PF_XMMI_INSTRUCTIONS_AVAILABLE)) {
    // SSE registers are located at offset 160 and MXCSR is at offset 24 in the
    // ExtendedRegisters block.
    uint8_t const *xmm_space =
        reinterpret_cast<uint8_t const *>(context.ExtendedRegisters);

    memcpy(&state.sse.mxcsr, xmm_space + 24, sizeof(state.sse.mxcsr));
    memcpy(&state.sse.mxcsrmask, xmm_space + 28, sizeof(state.sse.mxcsrmask));
    for (size_t n = 0; n < 8; n++) {
      static const int reg_size = sizeof(state.sse.regs[0]);
      memcpy(&state.sse.regs[n], xmm_space + 160 + n * reg_size, reg_size);
    }
  }

  return kSuccess;
}

ErrorCode Thread::writeCPUState(Architecture::CPUState const &state) {
  return kErrorUnsupported;
}
}
}
}
