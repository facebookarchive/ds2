//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/ARM64/Branching.h"

#include <cstdlib>

using namespace ds2::Architecture::ARM64;

namespace {

class ARM64Instruction {
  uint32_t _insn;

public:
  ARM64Instruction(uint32_t insn = 0) : _insn(insn) {}

  void reset(uint32_t insn = 0) { _insn = insn; }

  static inline int64_t SignExt(uint64_t n, size_t size) {
    size_t const sh = sizeof(n) * CHAR_BIT - size;
    return static_cast<int64_t>(n << sh) >> sh;
  }

  //
  // B <imm>
  // BL <imm>
  // B.<cond> <imm>
  //
  inline bool getB(BranchInfo &info) const {
    if ((_insn & 0x3c000000) == 0x14000000) {
      uint64_t disp;
      size_t size;
      if ((_insn >> 30) & 1) {
        info.type = kBranchTypeBcc;
        info.cond = static_cast<BranchCond>(_insn & 0xf);
        disp = (_insn & 0x00ffffe0) >> 5;
        size = 21;
      } else {
        if ((_insn >> 31) & 1) {
          info.type = kBranchTypeBL;
        } else {
          info.type = kBranchTypeB;
        }
        disp = _insn & 0x03ffffff;
        size = 28;
      }
      info.disp = SignExt(disp << 2, size);
      return true;
    }
    return false;
  }

  //
  // BR <reg>
  // BLR <reg>
  // RET <reg>
  //
  inline bool getBR(BranchInfo &info) const {
    if ((_insn & 0xfe1ffe1f) == 0xd61f0000) {
      if ((_insn >> 22) & 0xf) {
        info.type = kBranchTypeRET;
      } else if ((_insn >> 21) & 1) {
        info.type = kBranchTypeBLR;
      } else {
        info.type = kBranchTypeBR;
      }
      info.reg = (_insn & 0x0ff0) >> 5;

      return true;
    }
    return false;
  }

  //
  // CB{Z|NZ} <reg>, <imm>
  // TB{Z|NZ} <reg>, #offset, <imm>
  //
  inline bool getBZ(BranchInfo &info) const {
    if ((_insn & 0x7c000000) == 0x34000000) {
      info.reg = _insn & 0x1f;
      info.halfReg = !((_insn >> 31) & 1);

      if ((_insn >> 25) & 1) {
        info.type = kBranchTypeTB;
        info.disp = SignExt(((_insn & 0x007fffe0) >> 5) << 2, 16);
        info.offset = (_insn & 0x00f80000) >> 19;
        if (!info.halfReg) {
          info.offset += 32;
        }
      } else {
        info.type = kBranchTypeCB;
        info.disp = SignExt(((_insn & 0x00ffffe0) >> 5) << 2, 21);
      }

      return true;
    }
    return false;
  }

  bool getBranchInfo(BranchInfo &info) const {
    info.cond = kCondNV;
    info.halfReg = false;
    info.reg = -1;
    info.disp = 0;
    info.offset = 0;

    return (getB(info) || getBR(info) || getBZ(info));
  }
};
} // namespace

namespace ds2 {
namespace Architecture {
namespace ARM64 {

bool GetARM64BranchInfo(uint32_t insn, BranchInfo &info) {
  return ARM64Instruction(insn).getBranchInfo(info);
}
} // namespace ARM64
} // namespace Architecture
} // namespace ds2
