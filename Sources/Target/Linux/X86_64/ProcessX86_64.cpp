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

#include <cstdlib>

namespace X86_64Sys = ds2::Host::Linux::X86_64::Syscalls;

namespace ds2 {
namespace Target {
namespace Linux {

ErrorCode Process::allocateMemory(size_t size, uint32_t protection,
                                  uint64_t *address) {
  if (address == nullptr)
    return kErrorInvalidArgument;

  ProcessInfo info;
  ErrorCode error = getInfo(info);
  if (error != kSuccess)
    return error;

  U8Vector codestr;
  X86_64Sys::PrepareMmapCode(size, protection, codestr);

  //
  // Code inject and execute
  //
  error = ptrace().execute(_currentThread->tid(), info, &codestr[0],
                           codestr.size(), *address);
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

  U8Vector codestr;
  X86_64Sys::PrepareMunmapCode(address, size, codestr);

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
}
}
}
