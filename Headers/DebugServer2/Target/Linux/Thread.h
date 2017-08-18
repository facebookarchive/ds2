//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

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
