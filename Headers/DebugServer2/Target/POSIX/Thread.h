// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#pragma once

#include "DebugServer2/Target/ThreadBase.h"

namespace ds2 {
namespace Target {
namespace POSIX {

class Thread : public ds2::Target::ThreadBase {
protected:
  Thread(ds2::Target::Process *process, ThreadId tid);

public:
  ErrorCode readCPUState(Architecture::CPUState &state) override;
  ErrorCode writeCPUState(Architecture::CPUState const &state) override;

public:
  ErrorCode terminate() override;
  ErrorCode suspend() override;

public:
  ErrorCode step(int signal = 0, Address const &address = Address()) override;
  ErrorCode resume(int signal = 0, Address const &address = Address()) override;

protected:
  virtual ErrorCode updateStopInfo(int waitStatus);
};
} // namespace POSIX
} // namespace Target
} // namespace ds2
