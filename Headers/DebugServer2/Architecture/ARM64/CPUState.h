//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Architecture_ARM64_CPUState_h
#define __DebugServer2_Architecture_ARM64_CPUState_h

//
// Include the A32 variant
//
#include "DebugServer2/Architecture/ARM/CPUState.h"

namespace ds2 {
namespace Architecture {
namespace ARM64 {

//
// VFP is shared between A32 and A64
//
using ds2::Architecture::ARM::VFPSingle;
using ds2::Architecture::ARM::VFPDouble;
using ds2::Architecture::ARM::VFPQuad;

//
// Import the A32 variant
//
typedef ds2::Architecture::ARM::CPUState CPUState32;

//
// Define the 64-bit variant
//
struct CPUState64 {
  union {
    uint64_t regs[32 + 1];
    struct {
      uint64_t x0, x1, x2, r3, x4, x5, x6, r7, x8, x9, x10, r11, x12, x13, x14,
          r15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, sp,
          ip, lr, pc, cpsr;
    };
  } gp;

  union {
    VFPSingle sng[32];
    VFPDouble dbl[32];
    VFPQuad quad[16];
  } vfp;

  //
  // Accessors
  //
  inline uint64_t pc() const { return gp.pc; }
  inline void setPC(uint64_t pc) { gp.pc = pc; }

  inline uint64_t sp() const { return gp.sp; }
  inline void setSP(uint64_t sp) { gp.sp = sp; }
};

//
// Define the union of the two variants, this is the public
// structure.
//

struct CPUState {
  bool isA32; // Select which is valid between state32 and state64.

  union {
    CPUState32 state32;
    CPUState64 state64;
  };

  //
  // Accessors
  //
  inline uint64_t pc() const { return a32 ? state32.gp.pc : state64.gp.pc; }
  inline void setPC(uint64_t pc) {
    if (a32)
      state32.gp.pc = pc;
    else
      state64.gp.pc = pc;
  }

  inline uint64_t sp() const { return a32 ? state32.gp.sp : state64.gp.sp; }
  inline void setSP(uint64_t sp) {
    if (a32)
      state32.gp.sp = sp;
    else
      state64.gp.sp = sp;
  }

  inline uint64_t retval() const { return a32 ? state32.gp.r0 : state64.gp.r0; }

  inline bool isThumb() const { return a32 ? state32.isThumb() : false; }
};
}
}
}

#endif // !__DebugServer2_Architecture_ARM64_CPUState_h
