//
// Copyright (c) 2015, Facebook, Inc.
// Copyright (c) 2015, Corentin Derbois <cderbois@gmail.com>
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

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
