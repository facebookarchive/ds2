// Copyright (c) Meta Platforms, Inc. and affiliates.
// Copyright (c) 2015, Corentin Derbois <cderbois@gmail.com>
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#include "DebugServer2/Target/Darwin/Thread.h"
#include "DebugServer2/Architecture/CPUState.h"

namespace ds2 {
namespace Target {
namespace Darwin {

ErrorCode Thread::step(int signal, Address const &address) {
  // Darwin doesn't have a dedicated single-step call. We have to set the
  // single step flag (TF, 8th bit) in eflags and resume the thread.
  ErrorCode error = modifyRegisters([](Architecture::CPUState &state) {
    state.state64.gp.eflags |= (1 << 8);
  });
  if (error != kSuccess) {
    return error;
  }
  return resume(signal, address);
}

ErrorCode Thread::afterResume() {
  return modifyRegisters([](Architecture::CPUState &state) {
    state.state64.gp.eflags &= ~(1 << 8);
  });
}
} // namespace Darwin
} // namespace Target
} // namespace ds2
