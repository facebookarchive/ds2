// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#pragma once

#include "DebugServer2/Architecture/CPUState.h"
#include "DebugServer2/Core/BreakpointManager.h"
#include "DebugServer2/Target/Process.h"

namespace ds2 {
namespace Architecture {
namespace ARM {

ErrorCode PrepareThumbSoftwareSingleStep(Target::Process *process, uint32_t pc,
                                         Architecture::CPUState const &state,
                                         bool &link, uint32_t &nextPC,
                                         uint32_t &nextPCSize,
                                         uint32_t &branchPC,
                                         uint32_t &branchPCSize);

ErrorCode PrepareARMSoftwareSingleStep(Target::Process *process, uint32_t pc,
                                       Architecture::CPUState const &state,
                                       bool &link, uint32_t &nextPC,
                                       uint32_t &nextPCSize, uint32_t &branchPC,
                                       uint32_t &branchPCSize);

ErrorCode PrepareSoftwareSingleStep(Target::Process *process,
                                    BreakpointManager *manager,
                                    CPUState const &state,
                                    Address const &address);
} // namespace ARM
} // namespace Architecture
} // namespace ds2
