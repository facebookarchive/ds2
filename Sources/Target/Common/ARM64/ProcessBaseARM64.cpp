//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/ProcessBase.h"

using ds2::Architecture::GDBDescriptor;
using ds2::Architecture::LLDBDescriptor;

namespace ds2 {
namespace Target {

GDBDescriptor const *ProcessBase::getGDBRegistersDescriptor() const {
  return &Architecture::ARM64::GDB;
}

LLDBDescriptor const *ProcessBase::getLLDBRegistersDescriptor() const {
  return &Architecture::ARM64::LLDB;
}
} // namespace Target
} // namespace ds2
