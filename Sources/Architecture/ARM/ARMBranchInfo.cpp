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
        info.mode = DecodeShiftMode(static_cast<uint8_t>(info.disp), static_cast<uint8_t>((_insn >> 5) & 3));
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
        info.mode = DecodeShiftMode(static_cast<uint8_t>(info.disp), static_cast<uint8_t>((_insn >> 5) & 3));
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
      for (uint16_t regs = _insn & 0x7fff; regs != 0; regs = static_cast<uint16_t>((regs >> 1))) {
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
}

namespace ds2 {
namespace Architecture {
namespace ARM {

bool GetARMBranchInfo(uint32_t insn, BranchInfo &info) {
  return ARMInstruction(insn).getBranchInfo(info);
}
}
}
}

#ifdef TEST
static char const *const TypeNames[] = {
    "B_i",    "Bcc_i",  "CB_i",   "BX_r",   "BL_i",   "BLX_i",  "BLX_r",
    "MOV_pc", "LDR_pc", "LDM_pc", "POP_pc", "SUB_pc", "ADC_pc", "ADD_pc",
    "AND_pc", "BIC_pc", "EOR_pc", "ORR_pc", "RSB_pc", "RSC_pc", "SBC_pc",
    "MVN_pc", "ASR_pc", "LSL_pc", "LSR_pc", "ROR_pc", "RRX_pc",

};

static char const *const DispNames[] = {"Normal", "LSL", "LSR",
                                        "ASR",    "ROR", "RRX"};

static char const *const CondNames[] = {"EQ", "NE", "CS", "CC", "MI", "PL",
                                        "VS", "VC", "HI", "LS", "GE", "LT",
                                        "LE", "GT", "AL", "NV"};

static char const *const RegNames[] = {"R0", "R1", "R2", "R3", "R4", "R5",
                                       "R6", "R7", "R8", "R9", "SL", "FP",
                                       "IP", "SP", "LR", "PC"};

void PrintBranchInfo(BranchInfo const &info, uint32_t pc) {
  printf("PC %#x\n", pc);
  printf("\tType:%u [%s]\n", info.type, TypeNames[info.type]);
  printf("\tMode:%u [%s]\n", info.mode, DispNames[info.mode]);
  printf("\tCond:%u [%s]\n", info.cond, CondNames[info.cond]);
  printf("\tReg1:%d [%s]\n", info.reg1,
         info.reg1 < 0 ? "<NONE>" : RegNames[info.reg1]);
  printf("\tReg2:%d [%s%s]\n", info.reg2,
         info.reg2 < 0 ? "" : (info.subt ? "-" : "+"),
         info.reg2 < 0 ? "<NONE>" : RegNames[info.reg2]);
  printf("\tAlign:%u\n", (unsigned)info.align);
  printf("\tDisp:%d [%#x]\n", info.disp,
         (pc + info.disp + info.align - 1) & -info.align);
}

void Test(uint32_t insn, uint32_t pc) {
  ARMInstruction t(insn);
  BranchInfo info;
  if (t.getBranchInfo(info)) {
    PrintBranchInfo(info, pc);
  }
}

int main(int argc, char **argv) {
  Test(0xeaffffff, 0x4);  // B +4
  Test(0xea000000, 0xc);  // B +8
  Test(0xeafffffb, 0x10); // B -12

  Test(0x5affffff, 0x14); // BPL +4
  Test(0x4a000000, 0x1c); // BMI +8
  Test(0x0afffffb, 0x20); // BEQ -12

  Test(0xebffffff, 0x24); // BL +4
  Test(0xeb000000, 0x2c); // BL +8
  Test(0xebfffffb, 0x30); // BL -12

  Test(0x5bffffff, 0x34); // BLPL +4
  Test(0x4b000000, 0x3c); // BLMI +8
  Test(0x0bfffffb, 0x40); // BLEQ -12

  Test(0xfaffffff, 0x44); // BLX +4
  Test(0xfa000000, 0x4c); // BLX +8
  Test(0xfafffffb, 0x50); // BLX -12

  Test(0xfb000002, 0x54); // BLX +18
  Test(0xfa000002, 0x5c); // BLX +16
  Test(0xfbffffe6, 0x60); // BLX -94

  Test(0xe12fff1a, 0x64); // BX R10
  Test(0xe12fff3b, 0x68); // BLX R11

  Test(0x512fff1a, 0x64); // BXPL R10
  Test(0x012fff3b, 0x68); // BLXEQ R11

  Test(0xe51bf4d2, 0x70); // LDR PC, [R11, #-1234]
  Test(0xe59bf4d2, 0x74); // LDR PC, [R11, #+1234]

  Test(0x551bf4d2, 0x70); // LDRPL PC, [R11, #-1234]
  Test(0x059bf4d2, 0x74); // LDREQ PC, [R11, #+1234]

  Test(0xe41bf4d2, 0x78); // LDR PC, [R11], #-1234
  Test(0xe49bf4d2, 0x7c); // LDR PC, [R11], #+1234

  Test(0x541bf4d2, 0x80); // LDRPL PC, [R11], #-1234
  Test(0x049bf4d2, 0x84); // LDREQ PC, [R11], #+1234

  Test(0xe53bf4d2, 0x70); // LDR PC, [R11, #-1234]!
  Test(0xe5bbf4d2, 0x74); // LDR PC, [R11, #+1234]!

  Test(0x553bf4d2, 0x70); // LDRPL PC, [R11, #-1234]!
  Test(0x05bbf4d2, 0x74); // LDREQ PC, [R11, #+1234]!

  Test(0xe59ff00c, 0x78); // LDR PC, [PC, #12]
  Test(0xe51ff00c, 0x7c); // LDR PC, [PC, #-12]

  Test(0x559ff00c, 0x78); // LDRPL PC, [PC, #12]
  Test(0x051ff00c, 0x7c); // LDREQ PC, [PC, #-12]

  Test(0xe791f002, 0x80); // LDR PC, [R1, +R2]
  Test(0xe711f002, 0x84); // LDR PC, [R1, -R2]

  Test(0x5791f002, 0x80); // LDRPL PC, [R1, +R2]
  Test(0x0711f002, 0x84); // LDREQ PC, [R1, -R2]

  Test(0xe691f002, 0x88); // LDR PC, [R1], +R2
  Test(0xe611f002, 0x8c); // LDR PC, [R1], -R2

  Test(0x5691f002, 0x90); // LDRPL PC, [R1], +R2
  Test(0x0611f002, 0x94); // LDREQ PC, [R1], -R2

  Test(0xe7b1f002, 0x98); // LDR PC, [R1, +R2]!
  Test(0xe731f002, 0x9c); // LDR PC, [R1, -R2]!

  Test(0x57b1f002, 0xa0); // LDRPL PC, [R1, +R2]!
  Test(0x0731f002, 0xa4); // LDREQ PC, [R1, -R2]!

  Test(0xe791f102, 0xa8); // LDR PC, [R1, +R2, LSL #2]
  Test(0xe711f1a2, 0xac); // LDR PC, [R1, -R2, LSR #3]
  Test(0xe791f242, 0xb0); // LDR PC, [R1, +R2, ASR #4]
  Test(0xe711f262, 0xb4); // LDR PC, [R1, -R2, ROR #4]
  Test(0xe711f062, 0xb8); // LDR PC, [R1, -R2, RRX]

  Test(0xe8bd80f0, 0xc0); // POP {R4-R7,PC}
  Test(0xe8b18018, 0xc4); // LDMIA R1!, {R3-R4,PC}
  Test(0xe8328018, 0xc8); // LDMDA R2!, {R3-R4,PC}
  Test(0xe9368018, 0xcc); // LDMDB R6!, {R3-R4,PC}
  Test(0xe9b78018, 0xd0); // LDMIB R7!, {R3-R4,PC}

  Test(0xe3a0fc12, 0xd4); // MOV PC, #0x1200
  Test(0xe3b0fc12, 0xd8); // MOVS PC, #0x1200

  Test(0xe3e0fc12, 0xd4); // MVN PC, #0x1200
  Test(0xe3f0fc12, 0xd8); // MVNS PC, #0x1200

  Test(0xe1a0f00e, 0xdc); // MOV PC, LR
  Test(0xe1b0f00e, 0xe0); // MOVS PC, LR

  Test(0xe1e0f00e, 0xe4); // MVN PC, LR
  Test(0xe1f0f00e, 0xe8); // MVNS PC, LR

  Test(0xe28efc1f, 0xec); // ADD PC, LR, #0x1f00
  Test(0xe29efc1f, 0xf0); // ADDS PC, LR, #0x1f00

  Test(0xe08ef001, 0xf4); // ADD PC, LR, R1
  Test(0xe09ef001, 0xf8); // ADDS PC, LR, R1

  Test(0xe08ef061, 0xfc);  // ADD PC, LR, R1, RRX
  Test(0xe09ef061, 0x100); // ADDS PC, LR, R1, RRX

  Test(0xe0aef001, 0x104); // ADC PC, LR, R1
  Test(0xe0bef001, 0x108); // ADCS PC, LR, R1

  Test(0xe08ef001, 0x10c); // ADD PC, LR, R1
  Test(0xe09ef001, 0x110); // ADDS PC, LR, R1

  Test(0xe00ef001, 0x114); // AND PC, LR, R1
  Test(0xe01ef001, 0x118); // ANDS PC, LR, R1

  Test(0xe1cef001, 0x11c); // BIC PC, LR, R1
  Test(0xe1def001, 0x120); // BICS PC, LR, R1

  Test(0xe02ef001, 0x11c); // EOR PC, LR, R1
  Test(0xe03ef001, 0x120); // EORS PC, LR, R1

  Test(0xe18ef001, 0x11c); // ORR PC, LR, R1
  Test(0xe19ef001, 0x120); // ORRS PC, LR, R1

  Test(0xe06ef001, 0x11c); // RSB PC, LR, R1
  Test(0xe07ef001, 0x120); // RSBS PC, LR, R1

  Test(0xe0eef001, 0x11c); // RSC PC, LR, R1
  Test(0xe0fef001, 0x120); // RSCS PC, LR, R1

  Test(0xe0cef001, 0x11c); // SBC PC, LR, R1
  Test(0xe0def001, 0x120); // SBCS PC, LR, R1

  Test(0xe04ef001, 0x11c); // SUB PC, LR, R1
  Test(0xe05ef001, 0x120); // SUBS PC, LR, R1

  Test(0xe1a0f06e, 0x124); // RRX PC, LR
  Test(0xe1b0f06e, 0x128); // RRXS PC, LR

  Test(0xe1a0f0ae, 0x12c); // LSR PC, LR, #1
}
#endif
