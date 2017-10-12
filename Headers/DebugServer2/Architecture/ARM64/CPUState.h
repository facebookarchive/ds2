//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#pragma once

#if !defined(CPUSTATE_H_INTERNAL)
#error "You shall not include this file directly."
#endif

#include "DebugServer2/Architecture/ARM/CPUState.h" // Include the A32 variant
#include "DebugServer2/Architecture/ARM64/RegistersDescriptors.h"

namespace ds2 {
namespace Architecture {
namespace ARM64 {

//
// VFP is shared between A32 and A64
//
using ds2::Architecture::ARM::VFPDouble;
using ds2::Architecture::ARM::VFPQuad;
using ds2::Architecture::ARM::VFPSingle;

//
// Import the A32 variant
//
typedef ds2::Architecture::ARM::CPUState CPUState32;

//
// Define the 64-bit variant
//
struct CPUState64 {
  union {
    uint64_t regs[31 + 1 + 1 + 1];
    struct {
      uint64_t x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14,
          x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28,
          fp, lr, sp, pc, cpsr;
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

  inline uint64_t retval() const { return gp.x0; }

public:
  inline void getGPState(GPRegisterValueVector &regs) const {
    regs.clear();
    for (size_t n = 0; n < array_sizeof(gp.regs); n++) {
      regs.push_back(GPRegisterValue{sizeof(gp.regs[n]), gp.regs[n]});
    }
  }

  inline void setGPState(std::vector<uint64_t> const &regs) {
    for (size_t n = 0; n < regs.size() && n < array_sizeof(gp.regs); n++) {
      gp.regs[n] = regs[n];
    }
  }

public:
  inline void getStopGPState(GPRegisterStopMap &regs, bool forLLDB) const {
    if (forLLDB) {
      for (size_t n = 0; n < 33; n++) {
        regs[n + reg_lldb_x0] = GPRegisterValue{sizeof(gp.regs[n]), gp.regs[n]};
      }
      regs[reg_lldb_cpsr] = GPRegisterValue{sizeof(gp.cpsr), gp.cpsr};
    } else {
      // GDB can live with non-zero registers
      for (size_t n = 0; n < 33; n++) {
        if (n >= 13) {
          regs[n + reg_gdb_x0] =
              GPRegisterValue{sizeof(gp.regs[n]), gp.regs[n]};
        }
      }
      regs[reg_gdb_cpsr] = GPRegisterValue{sizeof(gp.cpsr), gp.cpsr};
    }
  }

public:
  inline bool getLLDBRegisterPtr(int regno, void **ptr, size_t *length) const {
    if (regno >= reg_lldb_x0 && regno <= reg_lldb_x30) {
      *ptr = const_cast<uint64_t *>(&gp.regs[regno - reg_lldb_x0]);
      *length = sizeof(gp.regs[0]);
    } else if (regno == reg_lldb_sp) {
      *ptr = const_cast<uint64_t *>(&gp.sp);
      *length = sizeof(gp.sp);
    } else if (regno == reg_lldb_pc) {
      *ptr = const_cast<uint64_t *>(&gp.pc);
      *length = sizeof(gp.pc);
    } else if (regno == reg_lldb_cpsr) {
      *ptr = const_cast<uint64_t *>(&gp.cpsr);
      *length = sizeof(gp.cpsr);
    } else {
      return false;
    }

    return true;
  }

  inline bool getGDBRegisterPtr(int regno, void **ptr, size_t *length) const {
    if (regno >= reg_gdb_x0 && regno <= reg_gdb_x30) {
      *ptr = const_cast<uint64_t *>(&gp.regs[regno - reg_gdb_x0]);
      *length = sizeof(gp.regs[0]);
    } else if (regno == reg_gdb_sp) {
      *ptr = const_cast<uint64_t *>(&gp.sp);
      *length = sizeof(gp.sp);
    } else if (regno == reg_gdb_pc) {
      *ptr = const_cast<uint64_t *>(&gp.pc);
      *length = sizeof(gp.pc);
    } else if (regno == reg_gdb_cpsr) {
      *ptr = const_cast<uint64_t *>(&gp.cpsr);
      *length = sizeof(gp.cpsr);
    } else {
      return false;
    }

    return true;
  }
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

  CPUState() : state64() {}

  //
  // Accessors
  //
  inline uint64_t pc() const {
    return isA32 ? static_cast<uint64_t>(state32.pc()) : state64.pc();
  }
  inline void setPC(uint64_t pc) {
    if (isA32)
      state32.setPC(pc);
    else
      state64.setPC(pc);
  }

  inline uint64_t sp() const {
    return isA32 ? static_cast<uint64_t>(state32.sp()) : state64.sp();
  }
  inline void setSP(uint64_t sp) {
    if (isA32)
      state32.setSP(sp);
    else
      state64.setSP(sp);
  }

  inline uint64_t retval() const {
    return isA32 ? static_cast<uint64_t>(state32.retval()) : state64.retval();
  }

  inline bool isThumb() const { return isA32 ? state32.isThumb() : false; }

public:
  inline void getGPState(GPRegisterValueVector &regs) const {
    if (isA32) {
      state32.getGPState(regs);
    } else {
      state64.getGPState(regs);
    }
  }

  inline void setGPState(std::vector<uint64_t> const &regs) {
    if (isA32) {
      state32.setGPState(regs);
    } else {
      state64.setGPState(regs);
    }
  }

public:
  inline void getStopGPState(GPRegisterStopMap &regs, bool forLLDB) const {
    if (isA32) {
      state32.getStopGPState(regs, forLLDB);
    } else {
      state64.getStopGPState(regs, forLLDB);
    }
  }

public:
  inline bool getLLDBRegisterPtr(int regno, void **ptr, size_t *length) const {
    if (isA32) {
      return state32.getLLDBRegisterPtr(regno, ptr, length);
    } else {
      return state64.getLLDBRegisterPtr(regno, ptr, length);
    }
  }

  inline bool getGDBRegisterPtr(int regno, void **ptr, size_t *length) const {
    if (isA32) {
      return state32.getGDBRegisterPtr(regno, ptr, length);
    } else {
      return state64.getGDBRegisterPtr(regno, ptr, length);
    }
  }
};
} // namespace ARM64
} // namespace Architecture
} // namespace ds2
