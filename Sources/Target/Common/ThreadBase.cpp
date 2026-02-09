// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#include "DebugServer2/Target/ThreadBase.h"
#include "DebugServer2/Target/Process.h"

namespace ds2 {
namespace Target {

ThreadBase::ThreadBase(Process *process, ThreadId tid)
    : _process(process), _tid(tid), _state(kStopped) {
  // When threads are created, they're stopped at the entry point waiting for
  // the debugger to continue them.
  _stopInfo.event = StopInfo::kEventStop;
  _stopInfo.reason = StopInfo::kReasonThreadEntry;
  _process->insert(this);
}

ErrorCode ThreadBase::modifyRegisters(
    std::function<void(Architecture::CPUState &state)> action) {
  Architecture::CPUState state;
  CHK(readCPUState(state));
  action(state);
  return writeCPUState(state);
}

ErrorCode ThreadBase::beforeResume() {
  BreakpointManager *bpm = _process->hardwareBreakpointManager();
  if (bpm != nullptr) {
    bpm->enable((Target::Thread *)this);
  }

  return kSuccess;
}
} // namespace Target
} // namespace ds2
