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

#include "DebugServer2/Architecture/ARM/SoftwareSingleStep.h"
#include "DebugServer2/Architecture/CPUState.h"
#include "DebugServer2/Core/BreakpointManager.h"
#include "DebugServer2/Target/Process.h"

namespace ds2 {
namespace Architecture {
namespace ARM64 {

ErrorCode PrepareARM64SoftwareSingleStep(Target::Process *process, uint64_t pc,
                                         Architecture::CPUState const &state,
                                         bool &link, uint64_t &nextPC,
                                         uint64_t &nextPCSize,
                                         uint64_t &branchPC,
                                         uint64_t &branchPCSize);
} // namespace ARM64
} // namespace Architecture
} // namespace ds2
