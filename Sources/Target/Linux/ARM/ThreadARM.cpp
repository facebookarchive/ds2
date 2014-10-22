//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#define __DS2_LOG_CLASS_NAME__ "Target::Thread"

#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Architecture/ARM/Branching.h"
#include "DebugServer2/BreakpointManager.h"
#include "DebugServer2/Log.h"

using ds2::Target::Linux::Thread;
using ds2::Target::Linux::Process;
using ds2::ErrorCode;

namespace {

static ErrorCode PrepareThumbSoftwareSingleStep(
    Process *process, uint32_t pc, ds2::Architecture::CPUState const &state,
    uint32_t &size, bool &link, uint32_t &branchPC, uint32_t &nextPC) {
  ErrorCode error;
  ds2::Architecture::ARM::BranchInfo info;
  uint32_t insns[2];

  error = process->readMemory(pc, insns, sizeof(insns));
  if (error != ds2::kSuccess)
    return error;

  if (!ds2::Architecture::ARM::GetThumbBranchInfo(insns, info))
    return ds2::kErrorUnknown;

  //
  // It's a branch, shrug, setup target breakpoint.
  //
  uint32_t address;

  DS2LOG(Architecture, Debug,
         "Thumb branch/IT found at %#lx (size=%zu,it=%s[%u])",
         (unsigned long)pc, info.size, info.it ? "true" : "false",
         info.it ? info.itCount : 0);

  branchPC = pc;

  //
  // If it's inside an IT block, we need to set the
  // branch after the IT block.
  //
  if (info.it) {
    nextPC += 2;
    branchPC += 2; // The branch is after the IT instruction

    //
    // We need to read all the instructions in the IT block
    // and skip past them.
    //
    uint16_t
        itinsns[4 * 2]; // there may be at most 4 instructions in the IT block

    error = process->readMemory(nextPC, itinsns, sizeof(itinsns));
    if (error != ds2::kSuccess)
      return error;

    size_t skip = 0;
    for (size_t n = 0; n < info.itCount; n++) {
      skip += static_cast<std::uint8_t>(
          ds2::Architecture::ARM::GetThumbInstSize(itinsns[skip]));
    }

    nextPC += (skip << 1);
    branchPC += (skip << 1);
  }

  link = (info.type == ds2::Architecture::ARM::kBranchTypeBL_i ||
          info.type == ds2::Architecture::ARM::kBranchTypeBLX_i ||
          info.type == ds2::Architecture::ARM::kBranchTypeBLX_r);

  //
  // If it's a conditional branch, we need to move
  // the nextPC by the size of this instruction.
  //
  if (link || info.type == ds2::Architecture::ARM::kBranchTypeBcc_i ||
      info.type == ds2::Architecture::ARM::kBranchTypeCB_i) {
    nextPC += info.size << 1;
  } else if (!info.it) {
    //
    // In all other case we don't need to set the
    // breakpoint on the following instruction.
    //
    nextPC = static_cast<uint32_t>(-1);
  }

  size = 2;
  switch (info.type) {
  //
  // None means a conditional instruction.
  //
  case ds2::Architecture::ARM::kBranchTypeNone:
    branchPC = static_cast<uint32_t>(-1);
    break;

  //
  // Simple branches
  //
  case ds2::Architecture::ARM::kBranchTypeB_i:
  case ds2::Architecture::ARM::kBranchTypeBL_i:
  case ds2::Architecture::ARM::kBranchTypeBcc_i:
  case ds2::Architecture::ARM::kBranchTypeCB_i:
    branchPC += info.disp;
    break;

  //
  // Branch to registers
  //
  case ds2::Architecture::ARM::kBranchTypeBX_r:
  case ds2::Architecture::ARM::kBranchTypeBLX_r:
  case ds2::Architecture::ARM::kBranchTypeMOV_pc:
    branchPC = state.gp.regs[info.reg1];
    break;

  //
  // Load and Branch
  //
  case ds2::Architecture::ARM::kBranchTypeLDR_pc:
    address = state.gp.regs[info.reg1];
    if (info.mode == ds2::Architecture::ARM::kBranchDispLSL) {
      address += state.gp.regs[info.reg2] << info.disp;
    } else {
      if (info.reg2 >= 0) {
        address += state.gp.regs[info.reg2];
      }
      address += info.disp;
    }

    error = process->readMemory(address, &branchPC, sizeof(branchPC));
    if (error != ds2::kSuccess)
      return error;
    break;

  case ds2::Architecture::ARM::kBranchTypeLDM_pc:
  case ds2::Architecture::ARM::kBranchTypePOP_pc:
    address = state.gp.regs[info.reg1] + info.disp;
    error = process->readMemory(address, &branchPC, sizeof(branchPC));
    if (error != ds2::kSuccess)
      return error;
    break;

  //
  // Switch to ARM has the added twist that the
  // breakpoint to be set is of ARM type and the
  // address shall be aligned.
  //
  case ds2::Architecture::ARM::kBranchTypeBLX_i:
    branchPC += info.disp;
    branchPC = (branchPC + info.align - 1) & -info.align;
    size = 4; // ARM branch
    break;

  //
  // ALU operations
  //
  case ds2::Architecture::ARM::kBranchTypeSUB_pc:
    branchPC = state.gp.regs[info.reg1] - info.disp;
    break;

  default:
    return ds2::kErrorUnsupported;
  }

  return ds2::kSuccess;
}

static ErrorCode PrepareARMSoftwareSingleStep(
    Process *process, uint32_t pc, ds2::Architecture::CPUState const &state,
    uint32_t &size, bool &link, uint32_t &branchPC, uint32_t &nextPC) {
  ErrorCode error;
  uint32_t insn;

  error = process->readMemory(pc, &insn, sizeof(insn));
  if (error != ds2::kSuccess)
    return error;

  ds2::Architecture::ARM::BranchInfo info;
  if (!ds2::Architecture::ARM::GetARMBranchInfo(insn, info))
    return ds2::kErrorUnknown;

  //
  // It's a branch, shrug, setup target breakpoint.
  //
  uint32_t address;

  DS2LOG(Architecture, Debug, "ARM branch found at %#lx",
         (unsigned long)state.gp.pc);

  branchPC = pc;

  link = (info.type == ds2::Architecture::ARM::kBranchTypeBL_i ||
          info.type == ds2::Architecture::ARM::kBranchTypeBLX_i ||
          info.type == ds2::Architecture::ARM::kBranchTypeBLX_r);

  //
  // If it's a conditional branch, we need to move
  // the nextPC by the size of this instruction.
  //
  if (info.cond != ds2::Architecture::ARM::kCondAL || link) {
    nextPC += info.size << 1;
  } else {
    //
    // In all other case we don't need to set the
    // breakpoint on the following instruction.
    //
    nextPC = static_cast<uint32_t>(-1);
  }

  size = 4;
  switch (info.type) {
  //
  // Simple branches
  //
  case ds2::Architecture::ARM::kBranchTypeB_i:
  case ds2::Architecture::ARM::kBranchTypeBL_i:
  case ds2::Architecture::ARM::kBranchTypeBcc_i:
    branchPC += info.disp;
    break;

  //
  // Branch to registers
  //
  case ds2::Architecture::ARM::kBranchTypeBX_r:
  case ds2::Architecture::ARM::kBranchTypeBLX_r:
    branchPC = state.gp.regs[info.reg1];
    break;

  case ds2::Architecture::ARM::kBranchTypeMOV_pc:
    switch (info.mode) {
    case ds2::Architecture::ARM::kBranchDispNormal:
      branchPC = state.gp.regs[info.reg1];
      break;

    default:
      return ds2::kErrorUnsupported;
    }
    break;

  //
  // Load and Branch
  //
  case ds2::Architecture::ARM::kBranchTypeLDR_pc:
    address = state.gp.regs[info.reg1];
    switch (info.mode) {
    case ds2::Architecture::ARM::kBranchDispLSL:
      address += state.gp.regs[info.reg2] << info.disp;
      break;
    case ds2::Architecture::ARM::kBranchDispNormal:
      if (!(info.reg2 < 0)) {
        address += state.gp.regs[info.reg2];
      }
      address += info.disp;
      break;
    default:
      return ds2::kErrorUnsupported;
    }

    error = process->readMemory(address, &branchPC, sizeof(branchPC));
    if (error != ds2::kSuccess)
      return error;

    //
    // We can switch to Thumb with an LDR pc. If
    // that's the case, adjust the target and size.
    //
    if (branchPC & 1) {
      branchPC &= ~1ULL;
      size = 2;
    }

    break;

  case ds2::Architecture::ARM::kBranchTypeLDM_pc:
  case ds2::Architecture::ARM::kBranchTypePOP_pc:
    address = state.gp.regs[info.reg1] + info.disp;
    error = process->readMemory(address, &branchPC, sizeof(branchPC));
    if (error != ds2::kSuccess)
      return error;
    break;

  //
  // Switch to Thumb has the added twist that the
  // breakpoint to be set is of Thumb type.
  //
  case ds2::Architecture::ARM::kBranchTypeBLX_i:
    branchPC += info.disp;
    size = 2; // Thumb branch
    break;

  //
  // ALU operations
  //
  case ds2::Architecture::ARM::kBranchTypeSUB_pc:
    branchPC = state.gp.regs[info.reg1] - info.disp;
    break;

  default:
    return ds2::kErrorUnsupported;
  }

  return ds2::kSuccess;
}
}

ErrorCode Thread::prepareSoftwareSingleStep(Address const &address) {
  Architecture::CPUState state;

  ErrorCode error = readCPUState(state);
  if (error != kSuccess)
    return error;

  bool link = false;
  bool isThumb = (state.gp.cpsr & (1 << 5));
  uint32_t pc = address.valid() ? address.value() : state.pc();
  uint32_t nextPC = pc;
  uint32_t branchPC = static_cast<uint32_t>(-1);
  uint32_t size = 0;

  //
  // We need to read 8 bytes, because of IT block
  //
  if (isThumb) {
    error = PrepareThumbSoftwareSingleStep(process(), pc, state, size, link,
                                           branchPC, nextPC);
  } else {
    error = PrepareARMSoftwareSingleStep(process(), pc, state, size, link,
                                         branchPC, nextPC);
  }

  DS2LOG(Architecture, Debug, "branchPC=%#lx[link=%s] nextPC=%#lx",
         (unsigned long)branchPC, link ? "true" : "false",
         (unsigned long)nextPC);

  if (branchPC != static_cast<uint32_t>(-1)) {
    error = process()->breakpointManager()->add(
        branchPC, BreakpointManager::kTypeTemporaryOneShot, size);
    if (error != kSuccess)
      return error;
  }

  if (nextPC != static_cast<uint32_t>(-1)) {
    if ((nextPC & ~1) == pc) {
      if (!isThumb) {
        nextPC += 4;
        size = 4;
      } else {
        uint32_t insn;

        error = process()->readMemory(pc & ~1, &insn, sizeof(insn));
        if (error != kSuccess)
          return error;

        auto inst_size = Architecture::ARM::GetThumbInstSize(insn);
        nextPC += static_cast<std::uint8_t>(inst_size);
        size = 2;
      }
    }

    error = process()->breakpointManager()->add(
        nextPC, BreakpointManager::kTypeTemporaryOneShot, size);
    if (error != kSuccess)
      return error;
  }

  return kSuccess;
}
