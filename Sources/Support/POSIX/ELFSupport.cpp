//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Support/POSIX/ELFSupport.h"

#include <elf.h>

namespace ds2 {
namespace Support {

bool ELFSupport::MachineTypeToCPUType(uint32_t machineType, bool is64Bit,
                                      CPUType &type, CPUSubType &subType) {
  switch (machineType) {
#if defined(ARCH_X86) || defined(ARCH_X86_64)
  case EM_386:
    type = kCPUTypeX86;
    subType = kCPUSubTypeX86_ALL;
    break;

#if defined(ARCH_X86_64)
  case EM_X86_64:
    type = kCPUTypeX86_64;
    subType = kCPUSubTypeX86_64_ALL;
    break;
#endif

#elif defined(ARCH_ARM) || defined(ARCH_ARM64)
  case EM_ARM:
    type = kCPUTypeARM;
    subType = kCPUSubTypeARM_ALL;
    break;

#if defined(ARCH_ARM64)
  case EM_AARCH64:
    type = kCPUTypeARM64;
    subType = kCPUSubTypeARM64_ALL;
    break;
#endif

#else
#error "Architecture not supported."
#endif

  default:
    type = kCPUTypeAny;
    subType = kCPUSubTypeInvalid;
    return false;
  }
  return true;
}
} // namespace Support
} // namespace ds2
