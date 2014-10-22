//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Target_ThreadBase_h
#define __DebugServer2_Target_ThreadBase_h

#include "DebugServer2/Architecture/CPUState.h"
#include "DebugServer2/Target/ProcessDecl.h"

namespace ds2 {
namespace Target {

class ThreadBase {
public:
  typedef std::map<ThreadId, Thread *> IdentityMap;

public:
  enum State { kInvalid, kRunning, kStepped, kStopped, kTerminated };

protected:
  Process *_process;
  ThreadId _tid;
  TrapInfo _trap;
  State _state;

protected:
  ThreadBase(Process *process, ThreadId tid);

public:
  virtual ~ThreadBase();

public:
  inline Process *process() const {
    return const_cast<ThreadBase *>(this)->_process;
  }
  inline ThreadId tid() const { return _tid; }
  inline TrapInfo const &trapInfo() const { return _trap; }

public:
  virtual ErrorCode terminate(int signal) = 0;

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
  virtual ErrorCode readCPUState(Architecture::CPUState &state) = 0;
  virtual ErrorCode writeCPUState(Architecture::CPUState const &state) = 0;

public:
  inline uint32_t core() const { return _trap.core; }

protected:
  friend class ProcessBase;
  virtual void updateState();

protected:
  virtual ErrorCode prepareSoftwareSingleStep(Address const &address);
};
}
}

#endif // !__DebugServer2_Target_ThreadBase_h
