//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/ARM/Branching.h"

#include <cstdlib>

using namespace ds2::Architecture::ARM;

namespace {

class ARMInstruction {
  uint32_t _insn;

public:
  ARMInstruction(uint32_t insn = 0) : _insn(insn) {}

  void reset(uint32_t insn = 0) { _insn = insn; }

  static inline int32_t SignExt(uint32_t n, size_t size) {
    size_t const sh = 32 - size;
    return static_cast<int32_t>(n << sh) >> sh;
  }

  static inline int32_t ExpandModifiedImmediate(uint32_t value) {
    size_t rotate = ((value >> 8) & 0xf) << 1;
    value &= 0xff;
    return (value >> rotate) | (value << (32 - rotate));
  }

  static inline BranchDisp DecodeShiftMode(uint8_t disp, uint8_t type) {
    switch (type & 3) {
    case 0:
      return kBranchDispLSL;
    case 1:
      return kBranchDispLSR;
    case 2:
      return kBranchDispASR;
    case 3:
      return (disp == 0) ? kBranchDispRRX : kBranchDispROR;
    }
    std::abort();
    return kBranchDispNormal;
  }

  //
  // B<cc> <imm>
  // BL<cc> <imm>
  // BLX<cc> <imm>
  //
  inline bool getB(BranchInfo &info) const {
    if ((_insn & 0x0e000000) == 0x0a000000) {
      uint32_t h = 0;

      info.cond = static_cast<BranchCond>(_insn >> 28);
      if (info.cond == kCondNV) {
        info.cond = kCondAL;
        info.type = kBranchTypeBLX_i;
        h = (_insn >> 24) & 1;
      } else if ((_insn >> 24) & 1) {
        info.type = kBranchTypeBL_i;
      } else {
        info.type = kBranchTypeB_i;
      }
      info.mode = kBranchDispNormal;
      info.reg1 = -1;
      info.reg2 = -1;
      info.disp = SignExt(((_insn & 0xffffff) << 2) | (h << 1), 26);
      // add 4 because the displacement is supposed to point AFTER
      // the branch instruction and again 4 for the pipeline
      info.disp += 8;
      return true;
    }
    return false;
  }

  //
  // BX <reg>
  // BLX <reg>
  //
  inline bool getBX(BranchInfo &info) const {
    if ((_insn & 0x0fffffd0) == 0x012fff10) { // BX/BLX
      info.cond = static_cast<BranchCond>(_insn >> 28);
      info.type = (_insn & 0x20) != 0 ? kBranchTypeBLX_r : kBranchTypeBX_r;
      info.mode = kBranchDispNormal;
      info.reg1 = _insn & 0xf;
      info.reg2 = -1;
      info.disp = 0;
      return true;
    }
    return false;
  }

  //
  // <opc1>S pc, <Rn>, #<const>
  // <opc1>S pc, <Rn>, <Rm>{, <shift>}
  // <opc2>S pc, #<const>
  // <opc2>S pc, <Rm>{, <shift>}
  // <opc3>S pc, <Rn>, #<const>
  // RRXS pc, <Rn>
  //
  // opc1 = ADC, ADD, AND, BIC, EOR, ORR, RSB, RSC, SBC, SUB
  // opc2 = MOV, MVN
  // opc3 = ASR, LSL, LSR, ROR
  //
  inline bool getALU_pc(BranchInfo &info) const {
    int form = 0;
    if ((_insn & 0x0e00f000) == 0x0200f000) {
      //
      // <opc1>S pc, <Rn>, #<const>
      // <opc2>S pc, #<const>
      //
      form = 1;
    } else if ((_insn & 0x0e00f010) == 0x0000f000) {
      //
      // <opc1>S pc, <Rn>, <Rm>{, <shift>}
      // <opc2>S pc, <Rm>{, <shift>}
      // <opc3>S pc, <Rn>, #<const>
      // RRXS pc, <Rn>
      //
      form = 2; // register form
    }

    if (form != 0) {
      info.cond = static_cast<BranchCond>(_insn >> 28);
      info.reg1 = (_insn >> 16) & 0xf;
      if (form == 1) {
        info.reg2 = -1;
        info.disp = ExpandModifiedImmediate(_insn & 0xfff);
        info.mode = kBranchDispNormal;
      } else {
        info.reg2 = _insn & 0xf;
        info.disp = (_insn >> 7) & 0x1f;
        info.mode = DecodeShiftMode(info.disp, (_insn >> 5) & 3);
      }

      switch ((_insn >> 21) & 0xf) {
      case 0:
        info.type = kBranchTypeAND_pc;
        break;
      case 1:
        info.type = kBranchTypeEOR_pc;
        break;
      case 2:
        info.type = kBranchTypeSUB_pc;
        break;
      case 3:
        info.type = kBranchTypeRSB_pc;
        break;
      case 4:
        info.type = kBranchTypeADD_pc;
        break;
      case 5:
        info.type = kBranchTypeADC_pc;
        break;
      case 6:
        info.type = kBranchTypeSBC_pc;
        break;
      case 7:
        info.type = kBranchTypeRSC_pc;
        break;
      case 12:
        info.type = kBranchTypeORR_pc;
        break;
      case 13:
        info.type = kBranchTypeMOV_pc;
        if (form == 1) {
          info.reg1 = -1;
        } else {
          info.reg1 = info.reg2;
          info.reg2 = -1;
        }
        break;
      case 14:
        info.type = kBranchTypeBIC_pc;
        break;
      case 15:
        info.type = kBranchTypeMVN_pc;
        if (form == 1) {
          info.reg1 = -1;
        }
        break;
      }
      if (info.disp == 0 && info.mode == kBranchDispLSL) {
        info.mode = kBranchDispNormal;
      }
      return true;
    }
    return false;
  }

  //
  // LDR pc, [reg, disp]
  //
  inline bool getLDR_pc(BranchInfo &info) const {
    //
    // LDR pc, [<Rn>{, #+/-<imm12>}]
    // LDR pc, [<Rn>], #+/-<imm12>
    // LDR pc, [<Rn>, #+/-<imm12>]!
    //
    if ((_insn & 0x0e50f000) == 0x0410f000) {
      info.type = kBranchTypeLDR_pc;
      info.cond = static_cast<BranchCond>(_insn >> 28);
      info.mode = kBranchDispNormal;
      info.reg1 = (_insn >> 16) & 0xf;
      info.reg2 = -1;
      info.disp = 0;
      // Add displacement only if pre-increment or indexing.
      if ((_insn >> 24) & 1) {
        info.disp = _insn & 0xfff;
      }
      if (((_insn >> 23) & 1) == 0) {
        info.disp = -info.disp;
      }
      return true;
    }

    //
    // LDR pc, <label>
    // LDR pc, [PC, #-0]
    //
    if ((_insn & 0x0f7ff000) == 0x051ff000) {
      info.type = kBranchTypeLDR_pc;
      info.cond = static_cast<BranchCond>(_insn >> 28);
      info.mode = kBranchDispNormal;
      info.reg1 = 15; // Program Counter
      info.reg2 = -1;
      info.disp = _insn & 0xfff;
      if (((_insn >> 23) & 1) == 0) {
        info.disp = -info.disp;
      }
      return true;
    }

    //
    // LDR pc, [<Rn>,+/-<Rm>{, <shift>}]{!}
    // LDR pc, [<Rn>],+/-<Rm>{, <shift>}
    //
    if ((_insn & 0x0e50f010) == 0x0610f000) {
      info.type = kBranchTypeLDR_pc;
      info.cond = static_cast<BranchCond>(_insn >> 28);
      info.reg1 = (_insn >> 16) & 0xf;
      info.reg2 = -1;
      info.mode = kBranchDispNormal;
      info.disp = 0;
      // Add displacement only if pre-increment or indexing.
      if ((_insn >> 24) & 1) {
        info.reg2 = _insn & 0xf;
        info.disp = (_insn >> 7) & 0x1f;
        info.mode = DecodeShiftMode(info.disp, (_insn >> 5) & 3);
      }
      // We need to know if we need to subtract the register #2
      info.subt = ((_insn >> 23) & 1) == 0;
      return true;
    }

    return false;
  }

  //
  // LDM{IA|IB|DA|DB} reg, {...,pc}
  //
  inline bool getLDM_pc(BranchInfo &info) const {
    if ((_insn & 0x0fd08000) == 0x08908000 || // LDMIA
        (_insn & 0x0fd08000) == 0x08108000 || // LDMDA
        (_insn & 0x0fd08000) == 0x09908000 || // LDMIB
        (_insn & 0x0fd08000) == 0x09108000) { // LDMDB
      uint32_t w = (_insn >> 21) & 1;
      uint32_t rn = (_insn >> 16) & 0xf;
      info.cond = static_cast<BranchCond>(_insn >> 28);
      info.type = ((_insn & 0x0fd00000) == 0x08900000 && w && rn == 13)
                      ? kBranchTypePOP_pc
                      : kBranchTypeLDM_pc;
      info.mode = kBranchDispNormal;
      info.reg1 = rn;
      info.reg2 = -1;
      // count bits
      info.disp = 0;
      for (uint16_t regs = _insn & 0x7fff; regs != 0; regs >>= 1) {
        info.disp += (regs & 1);
      }
      info.disp <<= 2;
      return true;
    }
    return false;
  }

  bool getBranchInfo(BranchInfo &info) const {
    info.subt = false;
    info.cond = kCondAL;
    info.mode = kBranchDispNormal;
    info.align = 1;
    info.reg1 = -1;
    info.reg2 = -1;
    info.disp = 0;

    return (getB(info) || getBX(info) || getALU_pc(info) || getLDR_pc(info) ||
            getLDM_pc(info));
  }
};
} // namespace

namespace ds2 {
namespace Architecture {
namespace ARM {

bool GetARMBranchInfo(uint32_t insn, BranchInfo &info) {
  return ARMInstruction(insn).getBranchInfo(info);
}
} // namespace ARM
} // namespace Architecture
} // namespace ds2
