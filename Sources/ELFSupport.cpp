//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/ELFSupport.h"

#include <elf.h>

using ds2::ELFSupport;

bool ELFSupport::MachineTypeToCPUType(uint32_t machineType, bool is64Bit,
                                      CPUType &type, CPUSubType &subType) {
  switch (machineType) {
#if defined(__i386__) || defined(__x86_64__)
  case EM_386:
    type = kCPUTypeX86;
    subType = kCPUSubTypeX86_ALL;
    break;

#if defined(__x86_64__)
  case EM_X86_64:
    type = kCPUTypeX86_64;
    subType = kCPUSubTypeX86_64_ALL;
    break;
#endif

#elif defined(__arm__) || defined(__aarch64__)
  case EM_ARM:
    type = kCPUTypeARM;
    subType = kCPUSubTypeARM_ALL;
    break;

#if defined(__aarch64__)
  case EM_AARCH64:
    type = kCPUTypeARM64;
    subType = kCPUSubTypeARM64_ALL;
    break;
#endif

#elif defined(__mips__) || defined(__mips64__)
  case EM_MIPS_RS3_LE:
    type = kCPUTypeMIPS;
    subType = kCPUSubTypeMIPS_ALL;
    break;

  case EM_MIPS:
#if !defined(__mips64__)
    if (!is64Bit)
#endif
    {
      type = is64Bit ? kCPUTypeMIPS64 : kCPUTypeMIPS;
      subType = kCPUSubTypeMIPS_ALL;
    }
    break;

#elif defined(__powerpc__) || defined(__powerpc64__)
  case EM_PPC:
    type = kCPUTypePOWERPC;
    subType = kCPUSubTypePOWERPC_ALL;
    break;

#if defined(__powerpc64__)
  case EM_PPC64:
    type = kCPUTypePOWERPC64;
    subType = kCPUSubTypePOWERPC_970;
    break;
#endif

#elif defined(__sparc__) || defined(__sparc64__)
  case EM_SPARC:
    type = kCPUTypeSPARC;
    subType = kCPUSubTypeSPARC_ALL;
    break;

#if defined(__sparc64__)
  case EM_SPARCV9:
  case EM_SPARC32PLUS:
    type = kCPUTypeSPARC64;
    subType = kCPUSubTypeSPARC_ALL;
    break;
#endif

#else
#error "architecture not supported."
#endif

  default:
    return false;
  }

  return true;
}
