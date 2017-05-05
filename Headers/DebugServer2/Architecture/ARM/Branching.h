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

#include <cstdint>

//
// Possible instructions that affect PC:
//
// ARM           | Thumb-2        | Thumb-1
// --------------+----------------+---------------
// b i           | b.n i          | b i
// bl i          | b.w i          | bl i
// blx i         | bl i           | bx r
// bx r          | blx i          | mov pc, ...
// blx r         | bx r           | pop {...,pc}
// ldr pc, ...   | blx r          |
// mov pc, ...   | cbz r, i       |
// pop {...,pc}  | cbnz r, i      |
// <aop> pc, ... | mov pc, ...    |
//               | ldr pc, ...    |
//               | pop {...,pc}   |
//               | ldm.w {...,pc} |
//
// aop = alu op (add, sub, big, etc...)
//

namespace ds2 {
namespace Architecture {
namespace ARM {

enum BranchType {
  //
  // ARM/Thumb
  //
  kBranchTypeNone,
  kBranchTypeB_i,
  kBranchTypeBcc_i,
  kBranchTypeCB_i,
  kBranchTypeBX_r,
  kBranchTypeBL_i,
  kBranchTypeBLX_i,
  kBranchTypeBLX_r,
  kBranchTypeMOV_pc,
  kBranchTypeLDR_pc,
  kBranchTypeLDM_pc,
  kBranchTypePOP_pc,
  kBranchTypeSUB_pc,
  kBranchTypeTBB,
  kBranchTypeTBH,

  //
  // ARM (legacy)
  //
  kBranchTypeADC_pc,
  kBranchTypeADD_pc,
  kBranchTypeAND_pc,
  kBranchTypeBIC_pc,
  kBranchTypeEOR_pc,
  kBranchTypeORR_pc,
  kBranchTypeRSB_pc,
  kBranchTypeRSC_pc,
  kBranchTypeSBC_pc,
  kBranchTypeMVN_pc,
  kBranchTypeASR_pc,
  kBranchTypeLSL_pc,
  kBranchTypeLSR_pc,
  kBranchTypeROR_pc,
  kBranchTypeRRX_pc,
};

enum BranchDisp {
  kBranchDispNormal,
  kBranchDispLSL,
  kBranchDispLSR,
  kBranchDispASR,
  kBranchDispROR,
  kBranchDispRRX
};

enum BranchCond {
  kCondEQ,
  kCondNE,
  kCondCS,
  kCondCC,
  kCondMI,
  kCondPL,
  kCondVS,
  kCondVC,
  kCondHI,
  kCondLS,
  kCondGE,
  kCondLT,
  kCondLE,
  kCondGT,
  kCondAL,
  kCondNV
};

enum class ThumbInstSize {
  TwoByteInst = 2,
  FourByteInst = 4,
};

struct BranchInfo {
  union {
    struct {
      uint32_t it : 1;
      uint32_t itCount : 3;
      uint32_t : 28;
    } /*thumb*/;
    struct {
      uint32_t subt : 1;
      uint32_t : 31;
    } /*arm*/;
  };
  BranchType type;
  BranchCond cond;
  BranchDisp mode;
  int32_t reg1;
  int32_t reg2;
  int32_t disp;
  size_t align;
};

bool GetARMBranchInfo(uint32_t insn, BranchInfo &info);

bool GetThumbBranchInfo(uint32_t const insn[2], BranchInfo &info);

ThumbInstSize GetThumbInstSize(uint32_t insn);
} // namespace ARM
} // namespace Architecture
} // namespace ds2
