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
