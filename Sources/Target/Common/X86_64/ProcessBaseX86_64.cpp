// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#include "DebugServer2/Target/ProcessBase.h"

using ds2::Architecture::GDBDescriptor;
using ds2::Architecture::LLDBDescriptor;

namespace ds2 {
namespace Target {

GDBDescriptor const *ProcessBase::getGDBRegistersDescriptor() const {
  if (_info.pointerSize == sizeof(uint32_t))
    return &Architecture::X86::GDB;
  else
    return &Architecture::X86_64::GDB;
}

LLDBDescriptor const *ProcessBase::getLLDBRegistersDescriptor() const {
  if (_info.pointerSize == sizeof(uint32_t))
    return &Architecture::X86::LLDB;
  else
    return &Architecture::X86_64::LLDB;
}
} // namespace Target
} // namespace ds2
