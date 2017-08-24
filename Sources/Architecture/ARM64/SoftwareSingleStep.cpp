//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/ARM64/SoftwareSingleStep.h"
#include "DebugServer2/Architecture/ARM64/Branching.h"
#include "DebugServer2/Utils/Bits.h"
#include "DebugServer2/Utils/Log.h"

using ds2::Architecture::CPUState;
using ds2::Target::Process;

namespace ds2 {
namespace Architecture {
namespace ARM64 {

ErrorCode PrepareARM64SoftwareSingleStep(Process *process, uint64_t pc,
                                         CPUState const &state, bool &link,
                                         uint64_t &nextPC, uint64_t &nextPCSize,
                                         uint64_t &branchPC,
                                         uint64_t &branchPCSize) {
  uint32_t insn;

  CHK(process->readMemory(pc, &insn, sizeof(insn)));
  BranchInfo info;
  if (!GetARM64BranchInfo(insn, info)) {
    // We couldn't find a branch, the next instruction is standard ARM64.
    nextPC = pc + sizeof(insn);
    nextPCSize = sizeof(insn);
    return kSuccess;
  }

  return kErrorUnsupported;
}
} // namespace ARM64
} // namespace Architecture
} // namespace ds2
