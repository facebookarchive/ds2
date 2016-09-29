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

#include "DebugServer2/Architecture/CPUState.h"
#include "DebugServer2/Target/ProcessDecl.h"
#include "DebugServer2/Utils/Log.h"

#include <functional>

namespace ds2 {
namespace Target {

class ThreadBase {
public:
  enum State { kInvalid, kRunning, kStepped, kStopped, kTerminated };

protected:
  Process *_process;
  ThreadId _tid;
  StopInfo _stopInfo;
  State _state;

protected:
  ThreadBase(Process *process, ThreadId tid);

public:
  virtual ~ThreadBase() = default;

public:
  inline Process *process() const { return _process; }
  inline ThreadId tid() const { return _tid; }
  inline StopInfo const &stopInfo() const { return _stopInfo; }

public:
  virtual ErrorCode terminate() = 0;

public:
  virtual ErrorCode suspend() = 0;

public:
  inline State state() const { return _state; }

public:
  virtual ErrorCode step(int signal = 0,
                         Address const &address = Address()) = 0;
  virtual ErrorCode resume(int signal = 0,
                           Address const &address = Address()) = 0;

public:
  virtual ErrorCode beforeResume();

public:
  virtual ErrorCode readCPUState(Architecture::CPUState &state) = 0;
  virtual ErrorCode writeCPUState(Architecture::CPUState const &state) = 0;
  virtual ErrorCode modifyRegisters(
      std::function<void(Architecture::CPUState &state)> action) final;

public:
  inline uint32_t core() const { return _stopInfo.core; }

protected:
  friend class ProcessBase;
  virtual void updateState() = 0;
};
} // namespace Target
} // namespace ds2
