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
#include "DebugServer2/Host/Linux/X86_64/Syscalls.h"
#include "DebugServer2/Target/Thread.h"

namespace X86_64Sys = ds2::Host::Linux::X86_64::Syscalls;

namespace ds2 {
namespace Target {
namespace Linux {

ErrorCode Process::allocateMemory(size_t size, uint32_t protection,
                                  uint64_t *address) {
  if (address == nullptr) {
    return kErrorInvalidArgument;
  }

  U8Vector codestr;
  X86_64Sys::PrepareMmapCode(size, protection, codestr);

  uint64_t result;
  ErrorCode error = executeCode(codestr, result);
  if (error != kSuccess) {
    return error;
  }

  // MAP_FAILED is -1.
  if (static_cast<int64_t>(result) == -1LL) {
    return kErrorNoMemory;
  }

  *address = result;
  return kSuccess;
}

ErrorCode Process::deallocateMemory(uint64_t address, size_t size) {
  if (size == 0) {
    return kErrorInvalidArgument;
  }

  U8Vector codestr;
  X86_64Sys::PrepareMunmapCode(address, size, codestr);

  uint64_t result;
  ErrorCode error = executeCode(codestr, result);
  if (error != kSuccess) {
    return error;
  }

  // Negative values returned by the kernel indicate failure.
  if (static_cast<int32_t>(result) < 0) {
    return kErrorInvalidArgument;
  }

  return kSuccess;
}
}
}
}
