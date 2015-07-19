//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Target_FreeBSD_Thread_h
#define __DebugServer2_Target_FreeBSD_Thread_h

#include "DebugServer2/Target/POSIX/Thread.h"

#include <csignal>

namespace ds2 {
namespace Target {
namespace FreeBSD {

class Thread : public ds2::Target::POSIX::Thread {
protected:
  friend class Process;
  Thread(Process *process, ThreadId tid);
  int _lastSyscallNumber;

public:
  virtual ~Thread();

public:
  virtual ErrorCode terminate();

public:
  virtual ErrorCode suspend();

public:
  virtual ErrorCode step(int signal = 0, Address const &address = Address());
  virtual ErrorCode resume(int signal = 0, Address const &address = Address());

public:
  virtual ErrorCode readCPUState(Architecture::CPUState &state);
  virtual ErrorCode writeCPUState(Architecture::CPUState const &state);

protected:
  virtual ErrorCode updateStopInfo(int waitStatus);
  virtual void updateState();

private:
  void updateState(bool force);

protected:
  virtual ErrorCode prepareSoftwareSingleStep(Address const &address);
};
}
}
}

#endif // !__DebugServer2_Target_FreeBSD_Thread_h
