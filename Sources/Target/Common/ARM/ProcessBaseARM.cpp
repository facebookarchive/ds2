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
  return &Architecture::ARM::GDB;
}

LLDBDescriptor const *ProcessBase::getLLDBRegistersDescriptor() const {
  return &Architecture::ARM::LLDB;
}
} // namespace Target
} // namespace ds2
