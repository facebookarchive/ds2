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
#include "DebugServer2/Utils/Bits.h"

using namespace ds2::Architecture::ARM;

namespace {

class ThumbInstruction {
  union {
    uint32_t i32[2];
    uint16_t i16[4];
  } _insn;

public:
  ThumbInstruction(uint32_t insn0 = 0, uint32_t insn1 = 0) {
    reset(insn0, insn1);
  }

  void reset(uint32_t insn0 = 0, uint32_t insn1 = 0) {
    _insn.i32[0] = insn0, _insn.i32[1] = insn1;
  }

  static inline bool InsnIsThumb1(uint32_t insn) {
    uint16_t lo = insn & 0xffff;
    return ((lo & 0xe000) != 0xe000 || (lo & 0x1800) == 0x0000);
  }

  static inline int32_t SignExt(uint32_t n, size_t size) {
    size_t const sh = 32 - size;
    return static_cast<int32_t>(n << sh) >> sh;
  }

  static inline int32_t MakeT2BranchDisp(uint32_t s, uint32_t j1, uint32_t j2,
                                         size_t immHSize, uint32_t immH,
                                         size_t immLSize, uint32_t immL,
                                         uint32_t zeroPad,
                                         bool xorValues = true) {
    size_t nbits = 0;
    uint32_t disp = 0;
    // It's not an error that j1/j2 are swapped when not xoring.
    uint32_t i1 = xorValues ? !(j1 ^ s) : j2;
    uint32_t i2 = xorValues ? !(j2 ^ s) : j1;

    disp |= s, disp <<= 1, nbits++;
    disp |= i1, disp <<= 1, nbits++;
    disp |= i2, disp <<= immHSize, nbits++;
    disp |= immH, disp <<= immLSize, nbits += immHSize;
    disp |= immL, disp <<= zeroPad, nbits += immLSize;
    nbits += zeroPad;

    return SignExt(disp, nbits);
  }

  //
  // IT <cond>
  //
  static inline bool GetIT(uint16_t insn, BranchInfo &info) {
    info.align = 1;
    info.it = (insn & 0xff00) == 0xbf00 && (insn & 0x00ff) != 0x0000;
    info.itCount = 0;
    if (info.it) {
      info.cond = static_cast<BranchCond>((insn >> 4) & 0xf);
      // itCount has 3 bits
      info.itCount = (5 - ds2::Utils::FFS(insn & 0xf)) & 7;
    } else {
      info.cond = kCondAL;
    }
    return info.it;
  }

  //
  // B.N <imm>
  //
  static inline bool GetB_N(uint16_t insn, BranchInfo &info) {
    if ((insn & 0xf800) == 0xe000) {
      info.type = kBranchTypeB_i;
      info.mode = kBranchDispNormal;
      info.reg1 = -1;
      info.reg2 = -1;
      info.disp = SignExt((insn & 0x7ff) << 1, 12);
      // add 4 because the displacement is supposed to point AFTER
      // the branch instruction plus 2 bytes for the pipeline.
      info.disp += 4;
      return true;
    }
    return false;
  }

  //
  // Bcc.N <imm>
  //
  static inline bool GetBcc_N(uint16_t insn, BranchInfo &info) {
    if ((insn & 0xf000) == 0xd000 && (insn & 0x0f00) < 0x0e00) {
      info.type = kBranchTypeBcc_i;
      info.cond = static_cast<BranchCond>((insn >> 8) & 0xf);
      info.mode = kBranchDispNormal;
      info.reg1 = -1;
      info.reg2 = -1;
      info.disp = SignExt((insn & 0xff) << 1, 9);
      // add 4 because the displacement is supposed to point AFTER
      // the branch instruction plus 2 bytes for the pipeline.
      info.disp += 4;
      return true;
    }
    return false;
  }

  //
  // BX <reg>
  //
  static inline bool GetBX(uint16_t insn, BranchInfo &info) {
    if ((insn & 0xff80) == 0x4700) {
      info.type = kBranchTypeBX_r;
      info.mode = kBranchDispNormal;
      info.reg1 = (insn >> 3) & 0xf;
      info.reg2 = -1;
      info.disp = 0;
      return true;
    }
    return false;
  }

  //
  // BLX <reg>
  //
  static inline bool GetBLX_r(uint16_t insn, BranchInfo &info) {
    if ((insn & 0xff80) == 0x4780) {
      info.type = kBranchTypeBLX_r;
      info.mode = kBranchDispNormal;
      info.reg1 = (insn >> 3) & 0xf;
      info.reg2 = -1;
      info.disp = 0;
      return true;
    }
    return false;
  }

  //
  // MOV pc, <reg>
  //
  static inline bool GetMOV_pc(uint16_t insn, BranchInfo &info) {
    // MOV pc, <reg>
    if ((insn & 0xff87) == 0x4687) {
      info.type = kBranchTypeMOV_pc;
      info.mode = kBranchDispNormal;
      info.reg1 = (insn >> 3) & 0xf;
      info.reg2 = -1;
      info.disp = 0;
      return true;
    }
    return false;
  }

  //
  // POP {...,pc}
  //
  static inline bool GetPOP_pc(uint16_t insn, BranchInfo &info) {
    if ((insn & 0xff00) == 0xbd00) {
      info.type = kBranchTypePOP_pc;
      info.mode = kBranchDispNormal;
      info.reg1 = 13; // Stack Pointer | TODO: Generate register names
      info.reg2 = -1;
      info.disp = ds2::Utils::PopCount(insn & 0xff) << 2;
      return true;
    }
    return false;
  }

  //
  // B.W <imm>
  //
  static inline bool GetB_W(uint16_t const *insn, BranchInfo &info) {
    if ((insn[0] & 0xf800) == 0xf000 && (insn[1] & 0xd000) == 0x9000) {
      info.type = kBranchTypeB_i;
      info.mode = kBranchDispNormal;
      info.reg1 = -1;
      info.reg2 = -1;

      uint32_t s = (insn[0] >> 10) & 1;
      uint32_t j1 = (insn[1] >> 13) & 1;
      uint32_t j2 = (insn[1] >> 11) & 1;
      uint32_t imm10 = insn[0] & 0x3ff;
      uint32_t imm11 = insn[1] & 0x7ff;

      info.disp = MakeT2BranchDisp(s, j1, j2, 10, imm10, 11, imm11, 1);

      // add 4 because the displacement is supposed to point AFTER
      // the branch instruction.
      info.disp += 4;

      return true;
    }
    return false;
  }

  //
  // Bcc.W <imm>
  //
  static inline bool GetBcc_W(uint16_t const *insn, BranchInfo &info) {
    if ((insn[0] & 0xf800) == 0xf000 && (insn[0] & 0x0380) != 0x0380 &&
        (insn[1] & 0xd000) == 0x8000) {
      info.type = kBranchTypeBcc_i;
      info.mode = kBranchDispNormal;
      info.reg1 = -1;
      info.reg2 = -1;
      info.cond = static_cast<BranchCond>((insn[0] >> 6) & 0xf);

      uint32_t s = (insn[0] >> 10) & 1;
      uint32_t j1 = (insn[1] >> 13) & 1;
      uint32_t j2 = (insn[1] >> 11) & 1;
      uint32_t imm6 = insn[0] & 0x3f;
      uint32_t imm11 = insn[1] & 0x7ff;

      info.disp = MakeT2BranchDisp(s, j1, j2, 6, imm6, 11, imm11, 1, false);

      // add 4 because the displacement is supposed to point AFTER
      // the branch instruction.
      info.disp += 4;

      return true;
    }
    return false;
  }

  //
  // BL <imm>
  //
  static inline bool GetBL(uint16_t const *insn, BranchInfo &info) {
    if ((insn[0] & 0xf800) == 0xf000 && (insn[1] & 0xd000) == 0xd000) {
      info.type = kBranchTypeBL_i;
      info.mode = kBranchDispNormal;
      info.reg1 = -1;
      info.reg2 = -1;

      uint32_t s = (insn[0] >> 10) & 1;
      uint32_t j1 = (insn[1] >> 13) & 1;
      uint32_t j2 = (insn[1] >> 11) & 1;
      uint32_t imm10 = insn[0] & 0x3ff;
      uint32_t imm11 = insn[1] & 0x7ff;

      info.disp = MakeT2BranchDisp(s, j1, j2, 10, imm10, 11, imm11, 1);

      // add 4 because the displacement is supposed to point AFTER
      // the branch instruction.
      info.disp += 4;

      return true;
    }
    return false;
  }

  //
  // BLX <imm>
  //
  static inline bool GetBLX_i(uint16_t const *insn, BranchInfo &info) {
    if ((insn[0] & 0xf800) == 0xf000 && (insn[1] & 0xd000) == 0xc000) {
      info.type = kBranchTypeBLX_i;
      info.mode = kBranchDispNormal;
      info.reg1 = -1;
      info.reg2 = -1;

      uint32_t s = (insn[0] >> 10) & 1;
      uint32_t j1 = (insn[1] >> 13) & 1;
      uint32_t j2 = (insn[1] >> 11) & 1;
      uint32_t imm10H = insn[0] & 0x3ff;
      uint32_t imm10L = (insn[1] >> 1) & 0x3ff;

      info.disp = MakeT2BranchDisp(s, j1, j2, 10, imm10H, 10, imm10L, 2);

      // the displacement must be aligned to 4, but it's up to the
      // user doing that because you need the PC.
      info.disp += 2;
      info.align = 4;

      return true;
    }
    return false;
  }

  //
  // CB{Z|NZ} reg, <imm>
  //
  static inline bool GetCBZ(uint16_t insn, BranchInfo &info) {
    if ((insn & 0xf500) == 0xb100) {
      info.type = kBranchTypeCB_i;
      info.mode = kBranchDispNormal;
      info.reg1 = -1;
      info.reg2 = -1;
      info.disp = ((((insn >> 9) & 1) << 5) | ((insn >> 3) & 0x1f)) << 1;
      // add 4 because the displacement is supposed to point AFTER
      // the branch instruction plus 2 bytes for the pipeline.
      info.disp += 4;
      return true;
    }
    return false;
  }

  //
  // LDR.W pc, [reg, disp]
  //
  static inline bool GetLDR_pc(uint16_t const *insn, BranchInfo &info) {
    // LDR.W pc, [Rn{, #<imm12>}]
    if ((insn[0] & 0xfff0) == 0xf8d0 && (insn[1] & 0xf000) == 0xf000) {
      info.type = kBranchTypeLDR_pc;
      info.mode = kBranchDispNormal;
      info.reg1 = insn[0] & 0xf;
      info.reg2 = -1;
      info.disp = insn[1] & 0xfff;
      return true;
    }

    // LDR.W pc, <label>
    // LDR.W pc, [pc, #-0]
    if ((insn[0] & 0xff7f) == 0xf85f && (insn[1] & 0xf000) == 0xf000) {
      info.type = kBranchTypeLDR_pc;
      info.mode = kBranchDispNormal;
      info.reg1 = 15; // Program Counter
      info.reg2 = -1;
      info.disp = 0;
      info.disp = insn[1] & 0xfff;
      if (((insn[0] >> 7) & 1) == 0) {
        info.disp = -info.disp;
      }
      return true;
    }

    // LDR.W pc, [Rn, Rm{, LSL #<imm2>}]
    if ((insn[0] & 0xfff0) == 0xf850 && (insn[1] & 0xffc0) == 0xf000) {
      info.type = kBranchTypeLDR_pc;
      info.reg1 = insn[0] & 0xf;
      info.reg2 = insn[1] & 0xf;
      info.disp = (insn[1] >> 4) & 3;
      if (info.disp == 0) {
        info.mode = kBranchDispNormal;
      } else {
        info.mode = kBranchDispLSL;
      }
      return true;
    }

    // LDR.W pc, [Rn, #-<imm8>]
    // LDR.W pc, [Rn] #+/-<imm8>
    // LDR.W pc, [Rn, #+/-<imm8>]!
    if ((insn[0] & 0xfff0) == 0xf850 && (insn[1] & 0xf800) == 0xf800) {
      info.type = kBranchTypeLDR_pc;
      info.mode = kBranchDispNormal;
      info.reg1 = insn[0] & 0xf;
      info.reg2 = -1;
      info.disp = 0;
      // Add displacement only if pre-increment or indexing.
      if ((insn[1] >> 10) & 1) {
        info.disp = insn[1] & 0xff;
        if (((insn[1] >> 9) & 1) == 0) {
          info.disp = -info.disp;
        }
      }
      return true;
    }

    return false;
  }

  //
  // LDM{IA|DB}.W reg, {...,pc}
  //
  static inline bool GetLDM_pc(uint16_t const *insn, BranchInfo &info) {
    if ((insn[0] & 0xffd0) == 0xe890 && (insn[1] & 0x8000) == 0x8000) {
      uint32_t rn = insn[0] & 0xf;
      uint32_t w = (insn[0] >> 5) & 1;
      info.type = (w && rn == 13) ? kBranchTypePOP_pc : kBranchTypeLDM_pc;
      info.mode = kBranchDispNormal;
      info.reg1 = rn;
      info.reg2 = -1;
      // count bits
      info.disp = 0;
      for (uint16_t regs = insn[1]; regs != 0; regs = static_cast<uint16_t>(regs >> 1)) {
        info.disp += (regs & 1);
      }
      // PC should be stored at address (reg1 + (bit_count - 1) * 4)
      info.disp = (info.disp - 1) * 4;
      return true;
    }
    return false;
  }

  //
  // SUBS pc, lr, #const
  //
  static inline bool GetSUBS_pc_lr(uint16_t const *insn, BranchInfo &info) {
    if (insn[0] == 0xf3de && (insn[1] & 0xff00) == 0x8f00) {
      info.type = kBranchTypeSUB_pc;
      info.mode = kBranchDispNormal;
      info.reg1 = insn[0] & 0xf;
      info.reg2 = -1;
      info.disp = insn[1] & 0xff;
      return true;
    }
    return false;
  }

  //
  // TBB [Rn, Rm]
  //
  static inline bool GetTBB(uint16_t const *insn, BranchInfo &info) {
    if ((insn[0] & 0xfff0) == 0xe8d0 && (insn[1] & 0xfff0) == 0xf000) {
      info.type = kBranchTypeTBB;
      info.mode = kBranchDispNormal;
      info.reg1 = insn[0] & 0xf;
      info.reg2 = insn[1] & 0xf;
      info.disp = 0;
      return true;
    }
    return false;
  }

  //
  // TBH [Rn, Rm, LSL #1]
  //
  static inline bool GetTBH(uint16_t const *insn, BranchInfo &info) {
    if ((insn[0] & 0xfff0) == 0xe8d0 && (insn[1] & 0xfff0) == 0xf010) {
      info.type = kBranchTypeTBH;
      info.mode = kBranchDispNormal;
      info.reg1 = insn[0] & 0xf;
      info.reg2 = insn[1] & 0xf;
      info.disp = 1;
      return true;
    }
    return false;
  }

  bool getBranchInfo(BranchInfo &info) const {
    uint16_t const *insn = _insn.i16;

    info.type = kBranchTypeNone;
    if (GetIT(*insn, info)) {
      // Note(sas): This looks wrong. If there is a branch, it is
      // supposed to be the last instruction in the IT block, therefore
      // we should get the total length of the IT block first, and work
      // on the last instruction.
      insn++;
    }

    bool isBranch = (
        // Thumb-1
        GetB_N(*insn, info) || GetBcc_N(*insn, info) || GetBL(insn, info) ||
        GetBLX_r(*insn, info) || GetBX(*insn, info) || GetMOV_pc(*insn, info) ||
        GetPOP_pc(*insn, info) ||
        // Thumb-2
        GetB_W(insn, info) || GetBcc_W(insn, info) || GetBLX_i(insn, info) ||
        GetCBZ(*insn, info) || GetLDR_pc(insn, info) || GetLDM_pc(insn, info) ||
        GetSUBS_pc_lr(insn, info) || GetTBB(insn, info) || GetTBH(insn, info));

    return (info.it || isBranch);
  }
};
}

namespace ds2 {
namespace Architecture {
namespace ARM {

bool GetThumbBranchInfo(uint32_t const insn[2], BranchInfo &info) {
  if (insn == nullptr)
    return false;
  else
    return ThumbInstruction(insn[0], insn[1]).getBranchInfo(info);
}

ThumbInstSize GetThumbInstSize(uint32_t insn) {
  return ThumbInstruction::InsnIsThumb1(insn) ? ThumbInstSize::TwoByteInst
                                              : ThumbInstSize::FourByteInst;
}
}
}
}

#ifdef TEST
static char const *const TypeNames[] = {
    "B_i",   "Bcc_i",  "CB_i",   "BX_r",   "BL_i",   "BLX_i",
    "BLX_r", "MOV_pc", "LDR_pc", "LDM_pc", "POP_pc", "SUBS_pc"};

static char const *const DispNames[] = {"Normal", "LSL"};

static char const *const CondNames[] = {"EQ", "NE", "CS", "CC", "MI", "PL",
                                        "VS", "VC", "HI", "LS", "GE", "LT",
                                        "LE", "GT", "AL", "NV"};

static char const *const RegNames[] = {"R0", "R1", "R2", "R3", "R4", "R5",
                                       "R6", "R7", "R8", "R9", "SL", "FP",
                                       "IP", "SP", "LR", "PC"};

void PrintBranchInfo(BranchInfo const &info, uint32_t pc) {
  printf("PC %#x\n", pc);
  printf("\tIT:%s\n", info.it ? "YES" : "NO");
  printf("\tType:%u [%s]\n", info.type, TypeNames[info.type]);
  printf("\tMode:%u [%s]\n", info.mode, DispNames[info.mode]);
  printf("\tCond:%u [%s]\n", info.cond, CondNames[info.cond]);
  printf("\tReg1:%d [%s]\n", info.reg1,
         info.reg1 < 0 ? "<NONE>" : RegNames[info.reg1]);
  printf("\tReg2:%d [%s]\n", info.reg2,
         info.reg2 < 0 ? "<NONE>" : RegNames[info.reg2]);
  printf("\tAlign:%u\n", (unsigned)info.align);
  printf("\tDisp:%d [%#x]\n", info.disp,
         (pc + info.disp + info.align - 1) & -info.align);
}

void TestT1(uint16_t insn, uint32_t pc) {
  union {
    uint16_t u16[2];
    uint32_t u32;
  } i;
  i.u16[0] = insn;
  i.u16[1] = 0;

  ThumbInstruction t(i.u32);
  BranchInfo info;
  if (t.getBranchInfo(info)) {
    PrintBranchInfo(info, pc);
  }
}

void TestT2(uint16_t insn1, uint16_t insn2, uint32_t pc) {
  union {
    uint16_t u16[2];
    uint32_t u32;
  } i;
  i.u16[0] = insn1;
  i.u16[1] = insn2;

  ThumbInstruction t(i.u32);
  BranchInfo info;
  if (t.getBranchInfo(info)) {
    PrintBranchInfo(info, pc);
  }
}

void TestT3(uint16_t insn1, uint16_t insn2, uint16_t insn3, uint32_t pc) {
  union {
    uint16_t u16[4];
    uint32_t u32[2];
  } i;
  i.u16[0] = insn1;
  i.u16[1] = insn2;
  i.u16[2] = insn3;
  i.u16[3] = 0;

  ThumbInstruction t(i.u32[0], i.u32[1]);
  BranchInfo info;
  if (t.getBranchInfo(info)) {
    PrintBranchInfo(info, pc);
  }
}

int main(int argc, char **argv) {
  TestT1(0xe7fe, 0x18d8);         // B.N   0
  TestT1(0xe402, 0x18ec);         // B.N   -2040
  TestT1(0xd4f8, 0x18e4);         // BMI.N -12
  TestT1(0xd5ff, 0x58);           // BPL.N +2
  TestT2(0xf7ff, 0xfffe, 0x18da); // BL    0
  TestT2(0xf7ff, 0xfffd, 0x18da); // BL    -2
  TestT2(0xf000, 0xf803, 0x18da); // BL    +10
  TestT1(0x4760, 0x7c);           // BX    IP
  TestT1(0x4798, 0x18e0);         // BLX   R3
  TestT1(0x46e7, 0x10f0);         // MOV   PC, IP
  TestT1(0xbd0f, 0x9e);           // POP   {R0-R3,PC}
  TestT2(0xf000, 0xb805, 0x5a);   // B.W   +14
  TestT2(0xf7ff, 0xbffa, 0x5a);   // B.W   -8
  TestT2(0xf140, 0x8003, 0x5e);   // BPL.W +10
  TestT2(0xf43f, 0xaffb, 0x5e);   // BEQ.W -6
  TestT2(0xf000, 0xe804, 0x72);   // BLX   +10
  TestT2(0xf000, 0xe800, 0x78);   // BLX   +10
  TestT2(0xf000, 0xe802, 0x2);    // BLX   +6
  TestT2(0xf000, 0xe802, 0x6);    // BLX   +6
  TestT2(0xf000, 0xe804, 0xa);    // BLX   +10
  TestT2(0xf7ff, 0xeff8, 0x12);   // BLX   -14
  TestT2(0xf7ff, 0xeff6, 0x16);   // BLX   -18
  TestT2(0xf7ff, 0xeff4, 0x1a);   // BLX   -22
  TestT1(0xb100, 0x88);           // CBZ   +4
  TestT1(0xb900, 0x8c);           // CBNZ  +4
  TestT2(0xf8d1, 0xffa0, 0x1e);   // LDR.W PC, [PC, #4000]
  TestT2(0xf8df, 0xf010, 0x2a);   // LDR.W PC, [PC, #16]
  TestT2(0xf851, 0xf002, 0x22);   // LDR.W PC, [R1, R2]
  TestT2(0xf851, 0xf032, 0x26);   // LDR.W PC, [R1, R2, LSL #3]
  TestT2(0xf8d3, 0xf018, 0x96);   // LDR.W PC, [R3, #24]
  TestT2(0xf851, 0xfc0d, 0x1e);   // LDR.W PC, [R1, #-13]
  TestT2(0xf851, 0xfb04, 0x32);   // LDR.W PC, [R1], #4
  TestT2(0xf851, 0xff04, 0x36);   // LDR.W PC, [R1, #4]!
  TestT2(0xe890, 0x9fff, 0x10dc); // LDMIA.W R0, {R0-R12,PC}
  TestT2(0xe8bd, 0x8fff, 0xa0);   // LDMIA.W SP!, {R0-R11,PC}
  TestT2(0xf3de, 0x8f0c, 0xa0);   // SUBS PC, LR, #12

  printf("---- IT BLOCKS ----\n");

  TestT2(0xbf08, 0xe7fe, 0x18d8);         // B.N   0
  TestT2(0xbf08, 0xe402, 0x18ec);         // B.N   -2040
  TestT2(0xbf08, 0xd4f8, 0x18e4);         // BMI.N -12
  TestT2(0xbf08, 0xd5ff, 0x58);           // BPL.N +2
  TestT3(0xbf08, 0xf7ff, 0xfffe, 0x18da); // BL    0
  TestT3(0xbf08, 0xf7ff, 0xfffd, 0x18da); // BL    -2
  TestT3(0xbf08, 0xf000, 0xf803, 0x18da); // BL    +10
  TestT2(0xbf08, 0x4760, 0x7c);           // BX    IP
  TestT2(0xbf08, 0x4798, 0x18e0);         // BLX   R3
  TestT2(0xbf08, 0x46e7, 0x10f0);         // MOV   PC, IP
  TestT2(0xbf08, 0xbd0f, 0x9e);           // POP   {R0-R3,PC}
  TestT3(0xbf08, 0xf000, 0xb805, 0x5a);   // B.W   +14
  TestT3(0xbf08, 0xf7ff, 0xbffa, 0x5a);   // B.W   -8
  TestT3(0xbf08, 0xf140, 0x8003, 0x5e);   // BPL.W +10
  TestT3(0xbf08, 0xf43f, 0xaffb, 0x5e);   // BEQ.W -6
  TestT3(0xbf08, 0xf000, 0xe804, 0x72);   // BLX   +10
  TestT3(0xbf08, 0xf000, 0xe800, 0x78);   // BLX   +10
  TestT3(0xbf08, 0xf000, 0xe802, 0x2);    // BLX   +6
  TestT3(0xbf08, 0xf000, 0xe802, 0x6);    // BLX   +6
  TestT3(0xbf08, 0xf000, 0xe804, 0xa);    // BLX   +10
  TestT3(0xbf08, 0xf7ff, 0xeff8, 0x12);   // BLX   -14
  TestT3(0xbf08, 0xf7ff, 0xeff6, 0x16);   // BLX   -18
  TestT3(0xbf08, 0xf7ff, 0xeff4, 0x1a);   // BLX   -22
  TestT2(0xbf08, 0xb100, 0x88);           // CBZ   +4
  TestT2(0xbf08, 0xb900, 0x8c);           // CBNZ  +4
  TestT3(0xbf08, 0xf8d1, 0xffa0, 0x1e);   // LDR.W PC, [PC, #4000]
  TestT3(0xbf08, 0xf8df, 0xf010, 0x2a);   // LDR.W PC, [PC, #16]
  TestT3(0xbf08, 0xf851, 0xf002, 0x22);   // LDR.W PC, [R1, R2]
  TestT3(0xbf08, 0xf851, 0xf032, 0x26);   // LDR.W PC, [R1, R2, LSL #3]
  TestT3(0xbf08, 0xf8d3, 0xf018, 0x96);   // LDR.W PC, [R3, #24]
  TestT3(0xbf08, 0xf851, 0xfc0d, 0x1e);   // LDR.W PC, [R1, #-13]
  TestT3(0xbf08, 0xf851, 0xfb04, 0x32);   // LDR.W PC, [R1], #4
  TestT3(0xbf08, 0xf851, 0xff04, 0x36);   // LDR.W PC, [R1, #4]!
  TestT3(0xbf08, 0xe890, 0x9fff, 0x10dc); // LDMIA.W R0, {R0-R12,PC}
  TestT3(0xbf08, 0xe8bd, 0x8fff, 0xa0);   // LDMIA.W SP!, {R0-R11,PC}
  TestT3(0xbf08, 0xf3de, 0x8f0c, 0xa0);   // SUBS PC, LR, #12
}
#endif
