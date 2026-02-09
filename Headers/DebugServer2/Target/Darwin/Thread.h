// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#pragma once

#include "DebugServer2/Target/POSIX/Thread.h"

#include <csignal>
#include <libproc.h>

#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <sys/types.h>

namespace ds2 {
namespace Target {
namespace Darwin {

class Thread : public ds2::Target::POSIX::Thread {
protected:
  int _lastSyscallNumber;

protected:
  friend class Process;
  Thread(Process *process, ThreadId tid);

protected:
  ErrorCode updateStopInfo(int waitStatus) override;
  void updateState() override;

public:
  virtual ErrorCode step(int signal,
                         Address const &address = Address()) override;

public:
  virtual ErrorCode afterResume();

public:
  virtual ErrorCode readCPUState(Architecture::CPUState &state) override;
  virtual ErrorCode writeCPUState(Architecture::CPUState const &state) override;

private:
  void updateState(bool force);
};
} // namespace Darwin
} // namespace Target
} // namespace ds2
