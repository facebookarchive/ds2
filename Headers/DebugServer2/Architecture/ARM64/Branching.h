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
// Conditional branch:
// b.cond i
// cbnz r, i
// cbz r, i
// tbnz r, #u, i
// tbz r, #u, i
//
// cond = condition code (eq, ne, cs, etc)
//
// Unconditional branch:
// b i
// bl i
// blr r
// br r
// ret {r}
//

namespace ds2 {
namespace Architecture {
namespace ARM64 {

enum BranchType {
  kBranchTypeNone,
  kBranchTypeB,
  kBranchTypeBL,
  kBranchTypeBcc,
  kBranchTypeBLR,
  kBranchTypeBR,
  kBranchTypeRET,
  kBranchTypeCB,
  kBranchTypeTB,
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
  kCondGT,
  kCondLE,
  kCondAL,
  kCondNV
};

struct BranchInfo {
  BranchType type;
  BranchCond cond; // only used by b.cond
  bool halfReg;    // true if w register, false if x register
  uint16_t reg;
  int64_t disp;
  uint16_t offset; // only used by tb{n|z}
};

bool GetARM64BranchInfo(uint32_t insn, BranchInfo &info);
} // namespace ARM64
} // namespace Architecture
} // namespace ds2
