// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#pragma once

#include "DebugServer2/Target/POSIX/Thread.h"

#include <csignal>

namespace ds2 {
namespace Target {
namespace Linux {

class Thread : public ds2::Target::POSIX::Thread {
protected:
  friend class Process;
  Thread(Process *process, ThreadId tid);

protected:
  ErrorCode updateStopInfo(int waitStatus) override;
  void updateState() override;
};
} // namespace Linux
} // namespace Target
} // namespace ds2
