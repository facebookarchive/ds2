//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Target_Windows_Thread_h
#define __DebugServer2_Target_Windows_Thread_h

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
  virtual ErrorCode terminate() { return kErrorUnsupported; }

public:
  virtual ErrorCode suspend() { return kErrorUnsupported; }

public:
  virtual ErrorCode step(int signal = 0, Address const &address = Address()) {
    return kErrorUnsupported;
  }
  virtual ErrorCode resume(int signal = 0, Address const &address = Address());

public:
  virtual ErrorCode readCPUState(Architecture::CPUState &state);
  virtual ErrorCode writeCPUState(Architecture::CPUState const &state);

protected:
  virtual void updateState();
  virtual void updateState(DEBUG_EVENT const &de);

protected:
  virtual ErrorCode updateTrapInfo() { return kErrorUnsupported; }
};
}
}
}

#endif // !__DebugServer2_Target_Windows_Thread_h
