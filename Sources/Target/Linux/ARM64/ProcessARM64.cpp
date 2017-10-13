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
#include "DebugServer2/Host/Linux/ARM64/Syscalls.h"
#include "DebugServer2/Target/Thread.h"

#include <cstdlib>
#include <sys/mman.h>
#include <sys/syscall.h>

namespace ARM64Sys = ds2::Host::Linux::ARM64::Syscalls;

namespace ds2 {
namespace Target {
namespace Linux {

static bool is32BitProcess(Process *process) {
  DS2ASSERT(process != nullptr);
  DS2ASSERT(process->currentThread() != nullptr);

  Architecture::CPUState state;
  CHK(process->currentThread()->readCPUState(state));

  return state.isA32;
}

ErrorCode Process::allocateMemory(size_t size, uint32_t protection,
                                  uint64_t *address) {
  if (address == nullptr || size == 0) {
    return kErrorInvalidArgument;
  }

  // We don't support ARM on ARM64 yet.
  DS2ASSERT(!is32BitProcess(this));

  ByteVector codestr;
  ARM64Sys::PrepareMmapCode(size, convertMemoryProtectionToPOSIX(protection),
                            codestr);

  uint64_t result;
  CHK(executeCode(codestr, result));
  CHK(checkMemoryErrorCode(result));

  *address = result;
  return kSuccess;
}

ErrorCode Process::deallocateMemory(uint64_t address, size_t size) {
  return kErrorUnsupported;
}
} // namespace Linux
} // namespace Target
} // namespace ds2
