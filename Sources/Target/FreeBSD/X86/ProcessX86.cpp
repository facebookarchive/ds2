//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Target/Thread.h"

//
// Include system header files for constants.
//
#include <cstdlib>
#include <sys/mman.h>
#include <sys/syscall.h>

using ds2::Architecture::GDBDescriptor;
using ds2::Architecture::LLDBDescriptor;

namespace ds2 {
namespace Target {
namespace Linux {

static uint8_t const gMmapCode[] = {
    0xb8, 0x00, 0x00, 0x00, 0x00, // 00: movl $sysno, %eax
    0x31, 0xdb,                   // 05: xorl %ebx, %ebx
    0xb9, 0x00, 0x00, 0x00, 0x00, // 07: movl $XXXXXXXX, %ecx
    0xba, 0x00, 0x00, 0x00, 0x00, // 0c: movl $XXXXXXXX, %edx
    0xbe, 0x00, 0x00, 0x00, 0x00, // 11: movl $XXXXXXXX, %esi
    0xbf, 0xff, 0xff, 0xff, 0xff, // 16: movl $-1, %edi
    0x31, 0xed,                   // 1b: xorl %ebp, %ebp
    0xcd, 0x80,                   // 1d: int  $0x80
    0xcc                          // 1f: int3
};

static uint8_t const gMunmapCode[] = {
    0xb8, 0x00, 0x00, 0x00, 0x00, // 00: movl $sysno, %eax
    0xbb, 0x00, 0x00, 0x00, 0x00, // 05: movl $XXXXXXXX, %ebx
    0xb9, 0x00, 0x00, 0x00, 0x00, // 0a: movl $XXXXXXXX, %ecx
    0xcd, 0x80,                   // 0f: int  $0x80
    0xcc                          // 10: int3
};

static void PrepareMmapCode(size_t size, int protection, ByteVector &codestr) {
  codestr.assign(&gMmapCode[0], &gMmapCode[sizeof(gMmapCode)]);

  uint8_t *code = &codestr[0];
  *reinterpret_cast<uint32_t *>(code + 0x01) = __NR_mmap2;
  *reinterpret_cast<uint32_t *>(code + 0x08) = size;
  *reinterpret_cast<uint32_t *>(code + 0x0d) = protection;
  *reinterpret_cast<uint32_t *>(code + 0x12) = MAP_ANON | MAP_PRIVATE;
}

static void PrepareMunmapCode(uint32_t address, size_t size,
                              ByteVector &codestr) {
  codestr.assign(&gMunmapCode[0], &gMunmapCode[sizeof(gMunmapCode)]);

  uint8_t *code = &codestr[0];
  *reinterpret_cast<uint32_t *>(code + 0x01) = __NR_munmap;
  *reinterpret_cast<uint32_t *>(code + 0x06) = address;
  *reinterpret_cast<uint32_t *>(code + 0x0b) = size;
}

ErrorCode Process::allocateMemory(size_t size, uint32_t protection,
                                  uint64_t *address) {
  if (address == nullptr) {
    return kErrorInvalidArgument;
  }

  ProcessInfo info;
  ErrorCode error = getInfo(info);
  if (error != kSuccess) {
    return error;
  }

  ByteVector codestr;
  PrepareMmapCode(size, convertMemoryProtectionToPOSIX(protection), codestr);

  // Code inject and execute
  error = ptrace().execute(_currentThread->tid(), info, &codestr[0],
                           codestr.size(), *address);
  if (error != kSuccess) {
    return error;
  }

  if (*address == (uint64_t)MAP_FAILED) {
    return kErrorNoMemory;
  }

  return kSuccess;
}

ErrorCode Process::deallocateMemory(uint64_t address, size_t size) {
  if (size == 0)
    return kErrorInvalidArgument;

  ProcessInfo info;
  ErrorCode error = getInfo(info);
  if (error != kSuccess)
    return error;

  ByteVector codestr;
  PrepareMunmapCode(address, size, codestr);

  //
  // Code inject and execute
  //
  uint64_t result = 0;
  error = ptrace().execute(_currentThread->tid(), info, &codestr[0],
                           codestr.size(), result);
  if (error != kSuccess)
    return error;

  if ((int)result < 0)
    return kErrorInvalidArgument;

  return kSuccess;
}
} // namespace Linux
} // namespace Target
} // namespace ds2
