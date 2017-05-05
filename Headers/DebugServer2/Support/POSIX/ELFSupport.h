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
