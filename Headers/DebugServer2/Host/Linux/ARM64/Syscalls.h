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

#include <asm-generic/unistd.h>
#include <sys/mman.h>

namespace ds2 {
namespace Host {
namespace Linux {
namespace ARM64 {
namespace Syscalls {

namespace {
static inline uint32_t MakeMovImmInstr(uint8_t reg, int32_t val) {
  DS2ASSERT(reg <= 31);
  DS2ASSERT(0 <= val && val <= 65535);
  static const uint32_t base = 0xd2800000;
  return base | (static_cast<uint16_t>(val) << 5) | reg;
}

static inline uint32_t MakeMovNegImmInstr(uint8_t reg, int32_t val) {
  DS2ASSERT(reg <= 31);
  DS2ASSERT(-65535 <= val && val <= 0);
  static const uint32_t base = 0x92800000;
  return base | (static_cast<uint16_t>(-val - 1) << 5) | reg;
}

static inline uint32_t MakeLdrRelInstr(uint8_t reg, int32_t offset) {
  DS2ASSERT(reg <= 31);
  DS2ASSERT(offset % 4 == 0);
  static const uint32_t base = 0x58000000;
  return base | (static_cast<uint16_t>(offset / 4) << 5) | reg;
}

static inline uint32_t MakeSvcInstr(uint16_t idx) {
  static const uint32_t base = 0xd4000001;
  return base | (idx << 5);
}

static inline uint32_t MakeBrkInstr(uint16_t idx) {
  static const uint32_t base = 0xd4200000;
  return base | (idx << 5);
}

template <typename T>
static inline void InsertBytes(ByteVector &codestr, T value) {
  auto valueBytes = reinterpret_cast<uint8_t *>(&value);
  codestr.insert(codestr.end(), valueBytes, valueBytes + sizeof(T));
}
} // namespace

static inline void PrepareMmapCode(size_t size, int protection,
                                   ByteVector &codestr) {
  static_assert(sizeof(size) == 8, "size_t should be 8-bytes long on ARM64");

  for (uint32_t instr : {
           MakeMovImmInstr(8, __NR_mmap),              // mov x8, __NR_mmap
           MakeMovImmInstr(0, 0),                      // mov x0, address
           MakeLdrRelInstr(1, 7 * sizeof(uint32_t)),   // ldr x1, <pc+28>
           MakeMovImmInstr(2, protection),             // mov x2, prot
           MakeMovImmInstr(3, MAP_ANON | MAP_PRIVATE), // mov x3, flags
           MakeMovNegImmInstr(4, -1),                  // mov x4, -1
           MakeMovImmInstr(5, 0),                      // mov x5, 0
           MakeSvcInstr(0),                            // svc #0
           MakeBrkInstr(0x100),                        // brk #0x100
       }) {
    InsertBytes(codestr, instr);
  }

  // Append the raw data that `MakeLdrRelInstr` instructions will reference.
  InsertBytes(codestr, size); // .quad XXXXXXXXXXXXXXXX
}

static inline void PrepareMunmapCode(uint64_t address, size_t size,
                                     ByteVector &codestr) {
  static_assert(sizeof(size) == 8, "size_t should be 8-bytes long on ARM64");

  for (uint32_t instr : {
           MakeMovImmInstr(8, __NR_munmap),          // mov x8, __NR_munmap
           MakeLdrRelInstr(0, 4 * sizeof(uint32_t)), // ldr x0, <pc+16>
           MakeLdrRelInstr(1, 5 * sizeof(uint32_t)), // ldr x1, <pc+20>
           MakeSvcInstr(0),                          // svc #0
           MakeBrkInstr(0x100),                      // brk #0x100
       }) {
    InsertBytes(codestr, instr);
  }

  // Append the raw data that `MakeLdrRelInstr` instructions will reference.
  InsertBytes(codestr, address); // .quad XXXXXXXXXXXXXXXX
  InsertBytes(codestr, size);    // .quad XXXXXXXXXXXXXXXX
}
} // namespace Syscalls
} // namespace ARM64
} // namespace Linux
} // namespace Host
} // namespace ds2
