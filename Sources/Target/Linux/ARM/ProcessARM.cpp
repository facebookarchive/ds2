//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/ARM/SoftwareBreakpointManager.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Utils/Log.h"

//
// Include system header files for constants.
//
#include <sys/syscall.h>
#include <sys/mman.h>
#include <cstdlib>

//
// TODO: Identify ARMv7 at runtime.
//

using ds2::Architecture::GDBDescriptor;
using ds2::Architecture::LLDBDescriptor;

namespace ds2 {
namespace Target {
namespace Linux {

namespace {

template <typename T>
static inline void InitCodeVector(U8Vector &codestr, T const &init) {
  uint8_t const *bptr = reinterpret_cast<uint8_t const *>(&init);
  uint8_t const *eptr = bptr + sizeof init;

  codestr.assign(bptr, eptr);
}

static inline void T1MOV8SetImmediate(uint16_t *insn, uint8_t value) {
  *insn &= 0xff00; // remove imm
  *insn |= value;
}

static inline void ARMMOV8SetImmediate(uint32_t *insn, uint8_t value) {
  *insn &= 0xffffff00; // remove imm
  *insn |= value;
}

#if (__ARM_ARCH >= 7)
//
// Thumb-2 code
//
static inline uint32_t __T2MOV16SetImmediate(uint32_t insn, uint16_t value) {
  insn &= ~0x70ff040f; // remove imm
  insn |= (value & 0xf000) >> 12;
  insn |= (value & 0x00ff) << 16;
  insn |= (value & 0x0800) >> 1;
  insn |= (value & 0x0700) << 20;
  return insn;
}

#if 0 // silence warning for unused function
static inline uint16_t __T2MOV16GetImmediate(uint32_t insn) {
  uint16_t value;
  insn &= 0x70ff040f; // keep imm
  value = (insn << 12) & 0xf000;
  value |= (insn >> 16) & 0x00ff;
  value |= (insn << 1) & 0x0800;
  value |= (insn >> 20) & 0x0700;
  return value;
}
#endif

static inline void T2MOVWTSetImmediate(uint64_t *insn, uint32_t value) {
  // Make sure insn is aligned to a 32-bit address.
  DS2ASSERT((uintptr_t)insn % 4 == 0);
  *insn = (((uint64_t)__T2MOV16SetImmediate(*insn >> 32, value >> 16) << 32) |
           __T2MOV16SetImmediate(*insn, value & 0xffff));
}

// We add the first "nop" to make the "movw; movt" pairs aligned to 32-bit word
// addresses, so that we can safely apply reinpterpret_cast<uint64_t *> for
// them and enable aggressive compiler optimizations (e.g., in release build,
// llvm will use vldr to load 64-bit double-words to VFP registers directly, in
// T2MOVWTSetImmediate() above).
static uint16_t const gT2MmapCode[] = {
    0xbf00,         // 00[00]: nop
    0x2000,         // 02[01]: movs   r0, #0
    0xf240, 0x0100, // 04[02]: movw   r1, #XXXX
    0xf2c0, 0x0100, // 08[04]: movt   r1, #XXXX
    0xf240, 0x0200, // 0c[06]: movw   r2, #XXXX
    0xf2c0, 0x0200, // 10[08]: movt   r2, #XXXX
    0xf240, 0x0300, // 14[0a]: movw   r3, #XXXX
    0xf2c0, 0x0300, // 18[0c]: movt   r3, #XXXX
    0xf05f, 0x34ff, // 1c[0e]: movs.w r4, #-1
    0x2500,         // 20[10]: movs   r5, #0
    0x2700,         // 22[11]: movs   r7, #XX
    0xdf00,         // 24[12]: svc    0
    0xde01,         // 26[13]: udf    #1
};

static uint16_t const gT2MunmapCode[] = {
    0xbf00,         // 00[00]: nop
    0x2000,         // 02[01]: movs   r0, #0
    0xf240, 0x0100, // 04[02]: movw   r1, #XXXX
    0xf2c0, 0x0100, // 08[04]: movt   r1, #XXXX
    0xf240, 0x0200, // 0c[06]: movw   r2, #XXXX
    0xf2c0, 0x0200, // 10[08]: movt   r2, #XXXX
    0x2700,         // 14[0a]: movs   r7, #XX
    0xdf00,         // 16[0b]: svc    0
    0xde01,         // 18[0c]: udf    #1
};

static void T2PrepareMmapCode(size_t size, uint32_t protection,
                              U8Vector &codestr) {
  InitCodeVector(codestr, gT2MmapCode);

  uint16_t *code = reinterpret_cast<uint16_t *>(&codestr[0]);
  T2MOVWTSetImmediate(reinterpret_cast<uint64_t *>(code + 0x02), size);
  T2MOVWTSetImmediate(reinterpret_cast<uint64_t *>(code + 0x06), protection);
  T2MOVWTSetImmediate(reinterpret_cast<uint64_t *>(code + 0x0a),
                      MAP_ANON | MAP_PRIVATE);
  T1MOV8SetImmediate(code + 0x11, __NR_mmap2);
}

static void T2PrepareMunmapCode(uint32_t address, size_t size,
                                U8Vector &codestr) {
  InitCodeVector(codestr, gT2MunmapCode);

  uint16_t *code = reinterpret_cast<uint16_t *>(&codestr[0]);
  T2MOVWTSetImmediate(reinterpret_cast<uint64_t *>(code + 0x02), address);
  T2MOVWTSetImmediate(reinterpret_cast<uint64_t *>(code + 0x06), size);
  T1MOV8SetImmediate(code + 0x0a, __NR_munmap);
}
#else
//
// Thumb-1 code
//
static uint16_t const gT1MmapCode[] = {
    0x2000,         // 00[00]: movs   r0, #0
    0x4904,         // 02[01]: ldr    r1, [pc, #16]
    0x4a04,         // 04[02]: ldr    r2, [pc, #16]
    0x4b05,         // 06[03]: ldr    r3, [pc, #20]
    0x2401,         // 08[04]: movs   r4, #1
    0x4264,         // 0a[05]: neg    r4, r4
    0x2500,         // 0c[06]: movs   r5, #0
    0x2700,         // 0e[07]: movs   r7, #XX
    0xdf00,         // 10[08]: svc    0
    0xde01,         // 12[09]: udf    #1
    0x0000, 0x0000, // 14[0a]: .word  XXXXXXXX
    0x0000, 0x0000, // 18[0c]: .word  XXXXXXXX
    0x0000, 0x0000  // 20[0e]: .word  XXXXXXXX
};

static uint16_t const gT1MunmapCode[] = {
    0x4802,         // 00[00]: ldr    r0, [pc, #8]
    0x4903,         // 02[01]: ldr    r1, [pc, #12]
    0x2700,         // 04[02]: movs   r7, #XX
    0xdf00,         // 06[03]: svc    0
    0xde01,         // 08[04]: udf    #1
    0x1c00,         // 0a[05]: nop
    0x0000, 0x0000, // 0c[06]: .word  XXXXXXXX
    0x0000, 0x0000  // 10[08]: .word  XXXXXXXX
};

static void T1PrepareMmapCode(size_t size, uint32_t protection,
                              U8Vector &codestr) {
  InitCodeVector(codestr, gT1MmapCode);

  uint16_t *code = reinterpret_cast<uint16_t *>(&codestr[0]);
  T1MOV8SetImmediate(code + 0x07, __NR_mmap2);
  *reinterpret_cast<uint32_t *>(code + 0x0a) = size;
  *reinterpret_cast<uint32_t *>(code + 0x0c) = protection;
  *reinterpret_cast<uint32_t *>(code + 0x0e) = MAP_ANON | MAP_PRIVATE;
}

static void T1PrepareMunmapCode(uint32_t address, size_t size,
                                U8Vector &codestr) {
  InitCodeVector(codestr, gT1MunmapCode);

  uint16_t *code = reinterpret_cast<uint16_t *>(&codestr[0]);
  T1MOV8SetImmediate(code + 0x02, __NR_munmap);
  *reinterpret_cast<uint32_t *>(code + 0x06) = address;
  *reinterpret_cast<uint32_t *>(code + 0x08) = size;
}
#endif

static void ThumbPrepareMmapCode(size_t size, uint32_t protection,
                                 U8Vector &codestr) {
#if (__ARM_ARCH >= 7)
  T2PrepareMmapCode(size, protection, codestr);
#else
  T1PrepareMmapCode(size, protection, codestr);
#endif
}

static void ThumbPrepareMunmapCode(uint32_t address, size_t size,
                                   U8Vector &codestr) {
#if (__ARM_ARCH >= 7)
  T2PrepareMunmapCode(address, size, codestr);
#else
  T1PrepareMunmapCode(address, size, codestr);
#endif
}

//
// ARM code
//
static uint32_t const gARMMmapCode[] = {
    0xe3b00000, // 00[00]: movs   r0, #0
    0xe59f1018, // 04[01]: ldr    r1, [pc, #24]
    0xe59f2018, // 08[02]: ldr    r2, [pc, #24]
    0xe59f3018, // 0c[03]: ldr    r3, [pc, #24]
    0xe3f04000, // 10[04]: mvns   r4, #0
    0xe3b05000, // 14[05]: movs   r5, #0
    0xe3b07000, // 18[06]: movs   r7, #XX
    0xef000000, // 1c[07]: svc    0
    0xe7f001f0, // 20[08]: udf    #16
    0x00000000, // 24[09]: .word  XXXXXXXX
    0x00000000, // 28[0a]: .word  XXXXXXXX
    0x00000000  // 30[0b]: .word  XXXXXXXX
};

static uint32_t const gARMMunmapCode[] = {
    0xe59f000c, // 00[00]: ldr    r0, [pc, #12]
    0xe59f100c, // 04[01]: ldr    r1, [pc, #12]
    0xe3b070c0, // 08[02]: movs   r7, #XX
    0xef000000, // 0c[03]: svc    0
    0xe7f001f0, // 10[04]: udf    #16
    0x00000000, // 14[05]: .word  XXXXXXXX
    0x00000000  // 18[06]: .word  XXXXXXXX
};

static void ARMPrepareMmapCode(size_t size, uint32_t protection,
                               U8Vector &codestr) {
  InitCodeVector(codestr, gARMMmapCode);

  uint32_t *code = reinterpret_cast<uint32_t *>(&codestr[0]);
  ARMMOV8SetImmediate(code + 0x06, __NR_mmap2);
  code[0x09] = size;
  code[0x0a] = protection;
  code[0x0b] = MAP_ANON | MAP_PRIVATE;
}

static void ARMPrepareMunmapCode(uint32_t address, size_t size,
                                 U8Vector &codestr) {
  InitCodeVector(codestr, gARMMunmapCode);

  uint32_t *code = reinterpret_cast<uint32_t *>(&codestr[0]);
  ARMMOV8SetImmediate(code + 0x02, __NR_munmap);
  code[0x05] = address;
  code[0x06] = size;
}
}

ErrorCode Process::allocateMemory(size_t size, uint32_t protection,
                                  uint64_t *address) {
  if (address == nullptr)
    return kErrorInvalidArgument;

  ProcessInfo info;
  ErrorCode error = getInfo(info);
  if (error != kSuccess)
    return error;

  //
  // We need to know if the process is running in Thumb or ARM mode.
  //
  Architecture::CPUState state;
  error = ptrace().readCPUState(_pid, info, state);
  if (error != kSuccess)
    return error;

  U8Vector codestr;
  if (state.isThumb()) {
    ThumbPrepareMmapCode(size, protection, codestr);
  } else {
    ARMPrepareMmapCode(size, protection, codestr);
  }

  //
  // Code inject and execute
  //
  error = ptrace().execute(_pid, info, &codestr[0], codestr.size(), *address);
  if (error != kSuccess)
    return error;

  if (*address == (uint64_t)MAP_FAILED)
    return kErrorNoMemory;

  return kSuccess;
}

ErrorCode Process::deallocateMemory(uint64_t address, size_t size) {
  if (size == 0)
    return kErrorInvalidArgument;

  ProcessInfo info;
  ErrorCode error = getInfo(info);
  if (error != kSuccess)
    return error;

  //
  // We need to know if the process is running in Thumb or ARM mode.
  //
  Architecture::CPUState state;
  error = ptrace().readCPUState(_pid, info, state);
  if (error != kSuccess)
    return error;

  U8Vector codestr;
  if (state.isThumb()) {
    ThumbPrepareMunmapCode(address, size, codestr);
  } else {
    ARMPrepareMunmapCode(address, size, codestr);
  }

  //
  // Code inject and execute
  //
  uint64_t result = 0;
  error = ptrace().execute(_pid, info, &codestr[0], codestr.size(), result);
  if (error != kSuccess)
    return error;

  if ((int)result < 0)
    return kErrorInvalidArgument;

  return kSuccess;
}

BreakpointManager *Process::breakpointManager() const {
  if (_breakpointManager == nullptr) {
    const_cast<Process *>(this)->_breakpointManager =
        new Architecture::ARM::SoftwareBreakpointManager(
            reinterpret_cast<Target::Process *>(const_cast<Process *>(this)));
  }

  return _breakpointManager;
}

WatchpointManager *Process::watchpointManager() const {
#if notyet
  //
  // TODO: this needs an hardware that supports hardware breakpoints,
  // I currently can't test this case, so it's #ifdefed out until
  // I have access to such hardware.
  //
  if (_watchpointManager == nullptr) {
    _watchpointManager = new Architecture::ARM::WatchpointManager(this);
  }

  return _watchpointManager;
#else
  return nullptr;
#endif
}

bool Process::isSingleStepSupported() const {
  //
  // Linux/ARM has no single step support, it must be emulated, this
  // is important for LLDB.
  //
  return false;
}

GDBDescriptor const *Process::getGDBRegistersDescriptor() const {
  return &Architecture::ARM::GDB;
}

LLDBDescriptor const *Process::getLLDBRegistersDescriptor() const {
  return &Architecture::ARM::LLDB;
}
}
}
}
