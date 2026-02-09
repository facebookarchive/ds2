// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#pragma once

#include "DebugServer2/Target/ThreadBase.h"

namespace ds2 {
namespace Target {
namespace Windows {

class Thread : public ds2::Target::ThreadBase {
protected:
  HANDLE _handle;

protected:
  friend class Process;
  Thread(Process *process, ThreadId tid, HANDLE handle);

public:
  virtual ~Thread();

public:
  virtual ErrorCode terminate() override;

public:
  virtual ErrorCode suspend() override;

public:
  virtual ErrorCode step(int signal = 0,
                         Address const &address = Address()) override;
  virtual ErrorCode resume(int signal = 0,
                           Address const &address = Address()) override;

public:
  virtual ErrorCode readCPUState(Architecture::CPUState &state) override;
  virtual ErrorCode writeCPUState(Architecture::CPUState const &state) override;

protected:
  virtual void updateState() override;
  virtual void updateState(DEBUG_EVENT const &de);
};
} // namespace Windows
} // namespace Target
} // namespace ds2
