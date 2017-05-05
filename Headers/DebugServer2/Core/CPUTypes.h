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

#include "DebugServer2/Base.h"
#include "DebugServer2/Constants.h"

namespace ds2 {

//
// CPU Type Flags
//

enum { kCPUArchABI64 = 0x01000000U };

//
// CPU Types
//

enum CPUType {
  kCPUTypeAny = (-1),
  kCPUTypeAll = 0,
  kCPUTypeAll64 = (kCPUTypeAll | kCPUArchABI64),

  kCPUTypeVAX = 1,
  kCPUTypeROMP = 2,
  // skip 3
  // skip 4
  // skip 5
  kCPUTypeMC680x0 = 6,
  kCPUTypeX86 = 7,
  kCPUTypeI386 = kCPUTypeX86,
  kCPUTypeX86_64 = (kCPUTypeX86 | kCPUArchABI64),
  kCPUTypeMIPS = 8,
  kCPUTypeMIPS64 = (kCPUTypeMIPS | kCPUArchABI64),
  // skip 9
  kCPUTypeMC98000 = 10,
  kCPUTypeHPPA = 11,
  kCPUTypeHPPA64 = (kCPUTypeHPPA | kCPUArchABI64),
  kCPUTypeARM = 12,
  kCPUTypeARM64 = (kCPUTypeARM | kCPUArchABI64),
  kCPUTypeMC88000 = 13,
  kCPUTypeSPARC = 14,
  kCPUTypeSPARC64 = (kCPUTypeSPARC | kCPUArchABI64),
  kCPUTypeI860 = 15,
  kCPUTypeALPHA = 16,
  // skip 17
  kCPUTypePOWERPC = 18,
  kCPUTypePOWERPC64 = (kCPUTypePOWERPC | kCPUArchABI64)
};

//
// CPU Sub Types.
//

enum CPUSubType {
  kCPUSubTypeInvalid = (-2),
  kCPUSubTypeMultiple = (-1),

  //
  // kCPUTypeVAX
  //

  kCPUSubTypeVAX_ALL = 0,
  kCPUSubTypeVAX780 = 1,
  kCPUSubTypeVAX785 = 2,
  kCPUSubTypeVAX750 = 3,
  kCPUSubTypeVAX730 = 4,
  kCPUSubTypeUVAXI = 5,
  kCPUSubTypeUVAXII = 6,
  kCPUSubTypeVAX8200 = 7,
  kCPUSubTypeVAX8500 = 8,
  kCPUSubTypeVAX8600 = 9,
  kCPUSubTypeVAX8650 = 10,
  kCPUSubTypeVAX8800 = 11,
  kCPUSubTypeUVAXIII = 12,

  //
  // kCPUTypeROMP
  //

  kCPUSubTypeROMP_ALL = 0,
  kCPUSubTypeRT_PC = 1,
  kCPUSubTypeRT_APC = 2,
  kCPUSubTypeRT_135 = 3,

  //
  // kCPUTypeMC680x0
  //

  kCPUSubTypeMC680x0_ALL = 1,
  kCPUSubTypeMC68030 = 1,
  kCPUSubTypeMC68040 = 2,
  kCPUSubTypeMC68030_ONLY = 3,

//
// kCPUTypeI386 (Legacy)
//

#define __CPUSubTypeINTEL(f, m) ((f) + ((m) << 4))

  kCPUSubTypeI386_ALL = __CPUSubTypeINTEL(3, 0),
  kCPUSubType386 = __CPUSubTypeINTEL(3, 0),
  kCPUSubType486 = __CPUSubTypeINTEL(4, 0),
  kCPUSubType486SX = __CPUSubTypeINTEL(4, 8),
  kCPUSubType586 = __CPUSubTypeINTEL(5, 0),
  kCPUSubTypePENT = __CPUSubTypeINTEL(5, 0),
  kCPUSubTypePENTPRO = __CPUSubTypeINTEL(6, 1),
  kCPUSubTypePENTII_M3 = __CPUSubTypeINTEL(6, 3),
  kCPUSubTypePENTII_M5 = __CPUSubTypeINTEL(6, 5),
  kCPUSubTypeCELERON = __CPUSubTypeINTEL(7, 6),
  kCPUSubTypeCELERON_MOBILE = __CPUSubTypeINTEL(7, 7),
  kCPUSubTypePENTIUM_3 = __CPUSubTypeINTEL(8, 0),
  kCPUSubTypePENTIUM_3_M = __CPUSubTypeINTEL(8, 1),
  kCPUSubTypePENTIUM_3_XEON = __CPUSubTypeINTEL(8, 2),
  kCPUSubTypePENTIUM_M = __CPUSubTypeINTEL(9, 0),
  kCPUSubTypePENTIUM_4 = __CPUSubTypeINTEL(10, 0),
  kCPUSubTypePENTIUM_4_M = __CPUSubTypeINTEL(10, 1),
  kCPUSubTypeITANIUM = __CPUSubTypeINTEL(11, 0),
  kCPUSubTypeITANIUM_2 = __CPUSubTypeINTEL(11, 1),
  kCPUSubTypeXEON = __CPUSubTypeINTEL(12, 0),
  kCPUSubTypeXEON_MP = __CPUSubTypeINTEL(12, 1),

#undef __CPUSubTypeINTEL

  //
  // kCPUTypeX86, kCPUTypeX86_64
  //

  kCPUSubTypeX86_ALL = 3,
  kCPUSubTypeX86_64_ALL = 3,
  kCPUSubTypeX86_ARCH1 = 4,

  //
  // kCPUTypeMIPS, kCPUTypeMIPS64
  //

  kCPUSubTypeMIPS_ALL = 0,
  kCPUSubTypeMIPS_R2300 = 1,
  kCPUSubTypeMIPS_R2600 = 2,
  kCPUSubTypeMIPS_R2800 = 3,
  kCPUSubTypeMIPS_R2000 = 4,
  kCPUSubTypeMIPS_R2000a = 5,
  kCPUSubTypeMIPS_R3000 = 6,
  kCPUSubTypeMIPS_R3000a = 7,

  //
  // kCPUTypeMC98000 (PowerPC)
  //

  kCPUSubTypeMC98000_ALL = 0,
  kCPUSubTypeMC98601 = 1,

  //
  // kCPUTypeHPPA, kCPUTypeHPPA64
  //

  kCPUSubTypeHPPA_ALL = 0,
  kCPUSubTypeHPPA_7100 = 0,
  kCPUSubTypeHPPA_7100LC = 1,

  //
  // kCPUTypeMC88000
  //

  kCPUSubTypeMC88000_ALL = 0,
  kCPUSubTypeMC88100 = 1,
  kCPUSubTypeMC88110 = 2,

  //
  // kCPUTypeSPARC, kCPUTypeSPARC64
  //

  kCPUSubTypeSPARC_ALL = 0,

  //
  // kCPUTypeI860
  //

  kCPUSubTypeI860_ALL = 0,
  kCPUSubTypeI860_860 = 1,

  //
  // kCPUTypePOWERPC, kCPUTypePOWERPC64
  //

  kCPUSubTypePOWERPC_ALL = 0,
  kCPUSubTypePOWERPC_601 = 1,
  kCPUSubTypePOWERPC_602 = 2,
  kCPUSubTypePOWERPC_603 = 3,
  kCPUSubTypePOWERPC_603e = 4,
  kCPUSubTypePOWERPC_603ev = 5,
  kCPUSubTypePOWERPC_604 = 6,
  kCPUSubTypePOWERPC_604e = 7,
  kCPUSubTypePOWERPC_620 = 8,
  kCPUSubTypePOWERPC_750 = 9,
  kCPUSubTypePOWERPC_7400 = 10,
  kCPUSubTypePOWERPC_7450 = 11,
  kCPUSubTypePOWERPC_970 = 100,

  //
  // kCPUTypeARM
  //

  kCPUSubTypeARM_ALL = 0,
  kCPUSubTypeARM_V4T = 5,
  kCPUSubTypeARM_V6 = 6,
  kCPUSubTypeARM_V5TEJ = 7,
  kCPUSubTypeARM_XSCALE = 8,
  kCPUSubTypeARM_V7 = 9,   // Cortex-A8 or successive
  kCPUSubTypeARM_V7F = 10, // Cortex-A9
  kCPUSubTypeARM_V7S = 11, // Apple A6
  kCPUSubTypeARM_V7K = 12,
  kCPUSubTypeARM_V8 = 13,   // Cortex-A53, Cortex-A57
  kCPUSubTypeARM_V6M = 14,  // Cortex-M0, Cortex-M0+, Cortex-M1
  kCPUSubTypeARM_V7M = 15,  // Cortex-M3
  kCPUSubTypeARM_V7EM = 16, // Cortex-M4

  //
  // kCPUTypeARM64
  //

  kCPUSubTypeARM64_ALL = 0,
  kCPUSubTypeARM64_V8 = 1
};

static inline bool CPUTypeIs64Bit(CPUType type) {
  return ((type & kCPUArchABI64) != 0 || type == kCPUTypeALPHA);
}

char const *GetCPUTypeName(CPUType type);
char const *GetArchName(CPUType type, CPUSubType subtype);
char const *GetArchName(CPUType type, CPUSubType subtype, Endian endian);
} // namespace ds2
