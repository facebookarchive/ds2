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

#include "DebugServer2/Architecture/X86/CPUState.h" // Include the X86 variant
#include "DebugServer2/Architecture/X86_64/RegistersDescriptors.h"

#include <cstring>

namespace ds2 {
namespace Architecture {
namespace X86_64 {

//
// Shared between X86 and X86-64
//
using ds2::Architecture::X86::AVXVector;
using ds2::Architecture::X86::SSEVector;
using ds2::Architecture::X86::X87Register;
using ds2::Architecture::X86::XFeature;

struct EAVXVector {
  uint64_t value[8]; // 512-bit values
};

//
// Import the X86 variant
//
typedef ds2::Architecture::X86::CPUState CPUState32;

//
// Define the 64-bit variant
//
struct CPUState64 {
  union {
    uint64_t regs[24];
    struct {
      uint64_t rax, rcx, rdx, rbx, rsi, rdi, rsp, rbp, r8, r9, r10, r11, r12,
          r13, r14, r15;
      uint64_t rip;
      struct {
        uint32_t cs;
        uint32_t : 32;
      };
      struct {
        uint32_t ss;
        uint32_t : 32;
      };
      struct {
        uint32_t ds;
        uint32_t : 32;
      };
      struct {
        uint32_t es;
        uint32_t : 32;
      };
      struct {
        uint32_t fs;
        uint32_t : 32;
      };
      struct {
        uint32_t gs;
        uint32_t : 32;
      };
      struct {
        uint32_t eflags;
        uint32_t : 32;
      };
    };
  } gp;

  struct {
    X87Register regs[8];
    uint16_t fstw;
    uint16_t fctw;
    uint16_t ftag;
    uint32_t fioff;
    uint32_t fiseg;
    uint32_t fooff;
    uint32_t foseg;
    uint16_t fop;
  } x87;

  union {
    struct {
      uint32_t mxcsr;
      uint32_t mxcsrmask;
      struct {
        // Dirty hack to map EAVX register locations
        union {
          EAVXVector _eavx[32];
          AVXVector _avx[64];
          SSEVector _sse[128];
        };
        SSEVector const &operator[](size_t index) const {
          return _sse[index << 2];
        }
        SSEVector &operator[](size_t index) { return _sse[index << 2]; }
      } regs;
    } sse;

    struct {
      uint32_t mxcsr;
      uint32_t mxcsrmask;
      struct {
        // Dirty hack to map EAVX register locations
        union {
          EAVXVector _eavx[32];
          AVXVector _avx[64];
        };
        AVXVector const &operator[](size_t index) const {
          return _avx[index << 1];
        }
        AVXVector &operator[](size_t index) { return _avx[index << 1]; }
      } regs;
    } avx;

    struct {
      uint32_t mxcsr;
      uint32_t mxcsrmask;
      EAVXVector regs[32];
    } eavx;
  };

  struct {
    uint64_t xfeatures_mask;
  } xsave_header;

  struct {
    uint64_t dr[8];
  } dr;

  uint64_t xcr0;

#if defined(OS_LINUX)
  struct {
    uint64_t orig_rax;
    uint64_t fs_base;
    uint64_t gs_base;
  } linux_gp;
#endif

public:
  CPUState64() { clear(); }

  inline void clear() {
    std::memset(&gp, 0, sizeof(gp));
    std::memset(&x87, 0, sizeof(x87));
    std::memset(&xsave_header, 0, sizeof(xsave_header));
    std::memset(&eavx, 0, sizeof(eavx));
    std::memset(&dr, 0, sizeof(dr));
    std::memset(&xcr0, 0, sizeof(xcr0));
#if defined(OS_LINUX)
    std::memset(&linux_gp, 0, sizeof(linux_gp));
#endif
  }

public:
  //
  // Accessors
  //
  inline uint64_t pc() const { return gp.rip; }
  inline void setPC(uint64_t pc) { gp.rip = pc; }

  inline uint64_t sp() const { return gp.rsp; }
  inline void setSP(uint64_t sp) { gp.rsp = sp; }

  inline uint64_t retval() const { return gp.rax; }

public:
#define _REGVALUE(REG)                                                         \
  GPRegisterValue { sizeof(gp.REG), gp.REG }
  //
  // These two functions interprets regs as GDB packed registers,
  // the order of GPR state, which is NOT the same order as the reg_gdb_*
  // (*sigh*)
  // is as follow:
  //
  // rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp, r8,
  // r9, r10, r11, r12, r13, r14, r15, rip,
  // eflags, cs, ss, ds, es, fs, gs
  //
  inline void getGPState(GPRegisterValueVector &regs) const {
    regs.clear();
    regs.push_back(_REGVALUE(rax));
    regs.push_back(_REGVALUE(rbx));
    regs.push_back(_REGVALUE(rcx));
    regs.push_back(_REGVALUE(rdx));
    regs.push_back(_REGVALUE(rsi));
    regs.push_back(_REGVALUE(rdi));
    regs.push_back(_REGVALUE(rbp));
    regs.push_back(_REGVALUE(rsp));
    regs.push_back(_REGVALUE(r8));
    regs.push_back(_REGVALUE(r9));
    regs.push_back(_REGVALUE(r10));
    regs.push_back(_REGVALUE(r11));
    regs.push_back(_REGVALUE(r12));
    regs.push_back(_REGVALUE(r13));
    regs.push_back(_REGVALUE(r14));
    regs.push_back(_REGVALUE(r15));
    regs.push_back(_REGVALUE(rip));
    regs.push_back(_REGVALUE(eflags));
    regs.push_back(_REGVALUE(cs));
    regs.push_back(_REGVALUE(ss));
    regs.push_back(_REGVALUE(ds));
    regs.push_back(_REGVALUE(es));
    regs.push_back(_REGVALUE(fs));
    regs.push_back(_REGVALUE(gs));
  }

  inline void setGPState(std::vector<uint64_t> const &regs) {
    gp.rax = regs[0];
    gp.rbx = regs[1];
    gp.rcx = regs[2];
    gp.rdx = regs[3];
    gp.rsi = regs[4];
    gp.rdi = regs[5];
    gp.rbp = regs[6];
    gp.rsp = regs[7];
    gp.r8 = regs[8];
    gp.r9 = regs[9];
    gp.r10 = regs[10];
    gp.r11 = regs[11];
    gp.r12 = regs[12];
    gp.r13 = regs[13];
    gp.r14 = regs[14];
    gp.r15 = regs[15];
    gp.rip = regs[16];
    gp.eflags = regs[17];
    gp.cs = regs[18];
    gp.ss = regs[19];
    gp.ds = regs[20];
    gp.es = regs[21];
    gp.fs = regs[22];
    gp.gs = regs[23];
  }

public:
  inline void getStopGPState(GPRegisterStopMap &regs, bool forLLDB) const {
    if (forLLDB) {
#define _SETREG(REG) regs[reg_lldb_##REG] = _REGVALUE(REG)
      _SETREG(rax);
      _SETREG(rcx);
      _SETREG(rdx);
      _SETREG(rbx);
      _SETREG(rsi);
      _SETREG(rdi);
      _SETREG(rsp);
      _SETREG(rbp);
      _SETREG(r8);
      _SETREG(r9);
      _SETREG(r10);
      _SETREG(r11);
      _SETREG(r12);
      _SETREG(r13);
      _SETREG(r14);
      _SETREG(r15);
      _SETREG(rip);
      _SETREG(eflags);
      _SETREG(cs);
      _SETREG(ss);
      _SETREG(ds);
      _SETREG(es);
      _SETREG(fs);
      _SETREG(gs);
#undef _SETREG
    } else {
#define _SETREG(REG) regs[reg_gdb_##REG] = _REGVALUE(REG)
      _SETREG(rax);
      _SETREG(rcx);
      _SETREG(rdx);
      _SETREG(rbx);
      _SETREG(rsi);
      _SETREG(rdi);
      _SETREG(rsp);
      _SETREG(rbp);
      _SETREG(r8);
      _SETREG(r9);
      _SETREG(r10);
      _SETREG(r11);
      _SETREG(r12);
      _SETREG(r13);
      _SETREG(r14);
      _SETREG(r15);
      _SETREG(rip);
      _SETREG(eflags);
      _SETREG(cs);
      _SETREG(ss);
      _SETREG(ds);
      _SETREG(es);
      _SETREG(fs);
      _SETREG(gs);
#undef _SETREG
    }
  }
#undef _REGVALUE

public:
  inline bool getLLDBRegisterPtr(int regno, void **ptr, size_t *length) const {
#define _GETREG2(T, REG, FLD)                                                  \
  case reg_lldb_##REG:                                                         \
    *ptr = &const_cast<CPUState64 *>(this)->T.FLD, *length = sizeof(T.FLD);    \
    break
#define _GETREG8L(T, REG, FREG)                                                \
  case reg_lldb_##REG:                                                         \
    *ptr =                                                                     \
        reinterpret_cast<uint8_t *>(&const_cast<CPUState64 *>(this)->T.FREG),  \
    *length = sizeof(uint8_t);                                                 \
    break
#define _GETREG8H(T, REG, FREG)                                                \
  case reg_lldb_##REG:                                                         \
    *ptr =                                                                     \
        reinterpret_cast<uint8_t *>(&const_cast<CPUState64 *>(this)->T.FREG) + \
        1,                                                                     \
    *length = sizeof(uint8_t);                                                 \
    break
#define _GETREG16(T, REG, FREG)                                                \
  case reg_lldb_##REG:                                                         \
    *ptr =                                                                     \
        reinterpret_cast<uint16_t *>(&const_cast<CPUState64 *>(this)->T.FREG), \
    *length = sizeof(uint16_t);                                                \
    break
#define _GETREG32(T, REG, FREG)                                                \
  case reg_lldb_##REG:                                                         \
    *ptr =                                                                     \
        reinterpret_cast<uint32_t *>(&const_cast<CPUState64 *>(this)->T.FREG), \
    *length = sizeof(uint32_t);                                                \
    break
#define _GETREG(T, REG) _GETREG2(T, REG, REG)

    switch (regno) {
      _GETREG(gp, rax);
      _GETREG(gp, rbx);
      _GETREG(gp, rcx);
      _GETREG(gp, rdx);
      _GETREG(gp, rsi);
      _GETREG(gp, rdi);
      _GETREG(gp, rsp);
      _GETREG(gp, rbp);
      _GETREG(gp, r8);
      _GETREG(gp, r9);
      _GETREG(gp, r10);
      _GETREG(gp, r11);
      _GETREG(gp, r12);
      _GETREG(gp, r13);
      _GETREG(gp, r14);
      _GETREG(gp, r15);
      _GETREG(gp, rip);
      _GETREG(gp, cs);
      _GETREG(gp, ss);
      _GETREG(gp, ds);
      _GETREG(gp, es);
      _GETREG(gp, fs);
      _GETREG(gp, gs);
      _GETREG(gp, eflags);

      _GETREG2(x87, st0, regs[0].data);
      _GETREG2(x87, st1, regs[1].data);
      _GETREG2(x87, st2, regs[2].data);
      _GETREG2(x87, st3, regs[3].data);
      _GETREG2(x87, st4, regs[4].data);
      _GETREG2(x87, st5, regs[5].data);
      _GETREG2(x87, st6, regs[6].data);
      _GETREG2(x87, st7, regs[7].data);
      _GETREG2(x87, fstat, fstw);
      _GETREG2(x87, fctrl, fctw);
      _GETREG(x87, ftag);
      _GETREG2(x87, fiseg, fiseg);
      _GETREG2(x87, fioff, fioff);
      _GETREG2(x87, foseg, foseg);
      _GETREG2(x87, fooff, fooff);
      _GETREG(x87, fop);

      _GETREG32(gp, eax, rax);
      _GETREG32(gp, ebx, rbx);
      _GETREG32(gp, ecx, rcx);
      _GETREG32(gp, edx, rdx);
      _GETREG32(gp, esi, rsi);
      _GETREG32(gp, edi, rdi);
      _GETREG32(gp, esp, rsp);
      _GETREG32(gp, ebp, rbp);
      _GETREG32(gp, r8d, r8);
      _GETREG32(gp, r9d, r9);
      _GETREG32(gp, r10d, r10);
      _GETREG32(gp, r11d, r11);
      _GETREG32(gp, r12d, r12);
      _GETREG32(gp, r13d, r13);
      _GETREG32(gp, r14d, r14);
      _GETREG32(gp, r15d, r15);

      _GETREG16(gp, ax, rax);
      _GETREG16(gp, bx, rbx);
      _GETREG16(gp, cx, rcx);
      _GETREG16(gp, dx, rdx);
      _GETREG16(gp, si, rsi);
      _GETREG16(gp, di, rdi);
      _GETREG16(gp, sp, rsp);
      _GETREG16(gp, bp, rbp);
      _GETREG16(gp, r8w, r8);
      _GETREG16(gp, r9w, r9);
      _GETREG16(gp, r10w, r10);
      _GETREG16(gp, r11w, r11);
      _GETREG16(gp, r12w, r12);
      _GETREG16(gp, r13w, r13);
      _GETREG16(gp, r14w, r14);
      _GETREG16(gp, r15w, r15);

      _GETREG8L(gp, al, rax);
      _GETREG8L(gp, bl, rbx);
      _GETREG8L(gp, cl, rcx);
      _GETREG8L(gp, dl, rdx);
      _GETREG8L(gp, sil, rsi);
      _GETREG8L(gp, dil, rdi);
      _GETREG8L(gp, spl, rsp);
      _GETREG8L(gp, bpl, rbp);
      _GETREG8L(gp, r8l, r8);
      _GETREG8L(gp, r9l, r9);
      _GETREG8L(gp, r10l, r10);
      _GETREG8L(gp, r11l, r11);
      _GETREG8L(gp, r12l, r12);
      _GETREG8L(gp, r13l, r13);
      _GETREG8L(gp, r14l, r14);
      _GETREG8L(gp, r15l, r15);

      _GETREG8H(gp, ah, rax);
      _GETREG8H(gp, bh, rbx);
      _GETREG8H(gp, ch, rcx);
      _GETREG8H(gp, dh, rdx);

      _GETREG(avx, mxcsr);
      _GETREG(avx, mxcsrmask);
      _GETREG2(avx, ymm0, regs[0]);
      _GETREG2(avx, ymm1, regs[1]);
      _GETREG2(avx, ymm2, regs[2]);
      _GETREG2(avx, ymm3, regs[3]);
      _GETREG2(avx, ymm4, regs[4]);
      _GETREG2(avx, ymm5, regs[5]);
      _GETREG2(avx, ymm6, regs[6]);
      _GETREG2(avx, ymm7, regs[7]);
      _GETREG2(avx, ymm8, regs[8]);
      _GETREG2(avx, ymm9, regs[9]);
      _GETREG2(avx, ymm10, regs[10]);
      _GETREG2(avx, ymm11, regs[11]);
      _GETREG2(avx, ymm12, regs[12]);
      _GETREG2(avx, ymm13, regs[13]);
      _GETREG2(avx, ymm14, regs[14]);
      _GETREG2(avx, ymm15, regs[15]);

      _GETREG2(sse, xmm0, regs[16]);
      _GETREG2(sse, xmm1, regs[17]);
      _GETREG2(sse, xmm2, regs[18]);
      _GETREG2(sse, xmm3, regs[19]);
      _GETREG2(sse, xmm4, regs[20]);
      _GETREG2(sse, xmm5, regs[21]);
      _GETREG2(sse, xmm6, regs[22]);
      _GETREG2(sse, xmm7, regs[23]);
      _GETREG2(sse, xmm8, regs[24]);
      _GETREG2(sse, xmm9, regs[25]);
      _GETREG2(sse, xmm10, regs[26]);
      _GETREG2(sse, xmm11, regs[27]);
      _GETREG2(sse, xmm12, regs[28]);
      _GETREG2(sse, xmm13, regs[29]);
      _GETREG2(sse, xmm14, regs[30]);
      _GETREG2(sse, xmm15, regs[31]);

    default:
      return false;
    }
#undef _GETREG16
#undef _GETREG8H
#undef _GETREG8L
#undef _GETREG
#undef _GETREG2

    return true;
  }

  inline bool getGDBRegisterPtr(int regno, void **ptr, size_t *length) const {
#define _GETREG2(T, REG, FLD)                                                  \
  case reg_gdb_##REG:                                                          \
    *ptr = &const_cast<CPUState64 *>(this)->T.FLD, *length = sizeof(T.FLD);    \
    break
#define _GETREG(T, REG) _GETREG2(T, REG, REG)

    switch (regno) {
      _GETREG(gp, rax);
      _GETREG(gp, rbx);
      _GETREG(gp, rcx);
      _GETREG(gp, rdx);
      _GETREG(gp, rsi);
      _GETREG(gp, rdi);
      _GETREG(gp, rsp);
      _GETREG(gp, rbp);
      _GETREG(gp, r8);
      _GETREG(gp, r9);
      _GETREG(gp, r10);
      _GETREG(gp, r11);
      _GETREG(gp, r12);
      _GETREG(gp, r13);
      _GETREG(gp, r14);
      _GETREG(gp, r15);
      _GETREG(gp, rip);
      _GETREG(gp, cs);
      _GETREG(gp, ss);
      _GETREG(gp, ds);
      _GETREG(gp, es);
      _GETREG(gp, fs);
      _GETREG(gp, gs);
      _GETREG(gp, eflags);

      _GETREG2(x87, st0, regs[0].data);
      _GETREG2(x87, st1, regs[1].data);
      _GETREG2(x87, st2, regs[2].data);
      _GETREG2(x87, st3, regs[3].data);
      _GETREG2(x87, st4, regs[4].data);
      _GETREG2(x87, st5, regs[5].data);
      _GETREG2(x87, st6, regs[6].data);
      _GETREG2(x87, st7, regs[7].data);
      _GETREG2(x87, fstat, fstw);
      _GETREG2(x87, fctrl, fctw);
      _GETREG(x87, ftag);
      _GETREG2(x87, fiseg, fiseg);
      _GETREG2(x87, fioff, fioff);
      _GETREG2(x87, foseg, foseg);
      _GETREG2(x87, fooff, fooff);
      _GETREG(x87, fop);

      // ymm0 maps to xmm0 for gdb
      _GETREG2(sse, ymm0, regs[0]);
      _GETREG2(sse, ymm1, regs[1]);
      _GETREG2(sse, ymm2, regs[2]);
      _GETREG2(sse, ymm3, regs[3]);
      _GETREG2(sse, ymm4, regs[4]);
      _GETREG2(sse, ymm5, regs[5]);
      _GETREG2(sse, ymm6, regs[6]);
      _GETREG2(sse, ymm7, regs[7]);
      _GETREG2(sse, ymm8, regs[8]);
      _GETREG2(sse, ymm9, regs[9]);
      _GETREG2(sse, ymm10, regs[10]);
      _GETREG2(sse, ymm11, regs[11]);
      _GETREG2(sse, ymm12, regs[12]);
      _GETREG2(sse, ymm13, regs[13]);
      _GETREG2(sse, ymm14, regs[14]);
      _GETREG2(sse, ymm15, regs[15]);

      _GETREG(sse, mxcsr);

#if defined(OS_LINUX)
      _GETREG(linux_gp, orig_rax);
#endif

    default:
      return false;
    }
#undef _GETREG
#undef _GETREG2

    return true;
  }
};

//
// Define the union of the two variants, this is the public
// structure.
//

struct CPUState {
  bool is32; // Select which is valid between state32 and state64.

  union {
    CPUState32 state32;
    CPUState64 state64;
  };

  CPUState() : state64() {}

  //
  // Accessors
  //
  inline uint64_t pc() const {
    return is32 ? static_cast<uint64_t>(state32.pc()) : state64.pc();
  }
  inline void setPC(uint64_t pc) {
    if (is32)
      state32.setPC(pc);
    else
      state64.setPC(pc);
  }

  inline uint64_t sp() const {
    return is32 ? static_cast<uint64_t>(state32.sp()) : state64.sp();
  }
  inline void setSP(uint64_t sp) {
    if (is32)
      state32.setSP(sp);
    else
      state64.setSP(sp);
  }

  inline uint64_t retval() const {
    return is32 ? static_cast<uint64_t>(state32.retval()) : state64.retval();
  }

public:
  inline void getGPState(GPRegisterValueVector &regs) const {
    if (is32) {
      state32.getGPState(regs);
    } else {
      state64.getGPState(regs);
    }
  }

  inline void setGPState(std::vector<uint64_t> const &regs) {
    if (is32) {
      state32.setGPState(regs);
    } else {
      state64.setGPState(regs);
    }
  }

public:
  inline void getStopGPState(GPRegisterStopMap &regs, bool forLLDB) const {
    if (is32) {
      state32.getStopGPState(regs, forLLDB);
    } else {
      state64.getStopGPState(regs, forLLDB);
    }
  }

public:
  inline bool getLLDBRegisterPtr(int regno, void **ptr, size_t *length) const {
    if (is32) {
      return state32.getLLDBRegisterPtr(regno, ptr, length);
    } else {
      return state64.getLLDBRegisterPtr(regno, ptr, length);
    }
  }

  inline bool getGDBRegisterPtr(int regno, void **ptr, size_t *length) const {
    if (is32) {
      return state32.getGDBRegisterPtr(regno, ptr, length);
    } else {
      return state64.getGDBRegisterPtr(regno, ptr, length);
    }
  }
};
} // namespace X86_64
} // namespace Architecture
} // namespace ds2
