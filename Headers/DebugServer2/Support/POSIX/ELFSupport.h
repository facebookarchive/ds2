// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#pragma once

#include "DebugServer2/Types.h"

namespace ds2 {
namespace Support {

class ELFSupport {
public:
  struct AuxiliaryVectorEntry {
    uint64_t type;
    uint64_t value;
  };

public:
  static bool MachineTypeToCPUType(uint32_t machineType, bool is64Bit,
                                   CPUType &type, CPUSubType &subType);
};
} // namespace Support
} // namespace ds2
