//
// Copyright (c) 2014, Facebook, Inc.
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

#elif defined(ARCH_MIPS) || defined(ARCH_MIPS64)
  case EM_MIPS_RS3_LE:
    type = kCPUTypeMIPS;
    subType = kCPUSubTypeMIPS_ALL;
    break;

  case EM_MIPS:
#if !defined(ARCH_MIPS64)
    if (!is64Bit)
#endif
    {
      type = is64Bit ? kCPUTypeMIPS64 : kCPUTypeMIPS;
      subType = kCPUSubTypeMIPS_ALL;
    }
    break;

#elif defined(ARCH_PPC) || defined(ARCH_PPC64)
  case EM_PPC:
    type = kCPUTypePOWERPC;
    subType = kCPUSubTypePOWERPC_ALL;
    break;

#if defined(ARCH_PPC64)
  case EM_PPC64:
    type = kCPUTypePOWERPC64;
    subType = kCPUSubTypePOWERPC_970;
    break;
#endif

#elif defined(ARCH_SPARC) || defined(ARCH_SPARC64)
  case EM_SPARC:
    type = kCPUTypeSPARC;
    subType = kCPUSubTypeSPARC_ALL;
    break;

#if defined(ARCH_SPARC64)
  case EM_SPARCV9:
  case EM_SPARC32PLUS:
    type = kCPUTypeSPARC64;
    subType = kCPUSubTypeSPARC_ALL;
    break;
#endif

#else
#error "Architecture not supported."
#endif

  default:
    return false;
  }

  return true;
}
}
