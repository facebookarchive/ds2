//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Target_Linux_Thread_h
#define __DebugServer2_Target_Linux_Thread_h

#include "DebugServer2/Target/POSIX/Thread.h"

#include <csignal>

namespace ds2 {
namespace Target {
namespace Linux {

class Thread : public ds2::Target::POSIX::Thread {
protected:
  friend class Process;
  Thread(Process *process, ThreadId tid);

public:
  ~Thread() override;

public:
  ErrorCode terminate() override;

public:
  ErrorCode suspend() override;

public:
  ErrorCode step(int signal = 0, Address const &address = Address()) override;
  ErrorCode resume(int signal = 0, Address const &address = Address()) override;

public:
  ErrorCode readCPUState(Architecture::CPUState &state) override;
  ErrorCode writeCPUState(Architecture::CPUState const &state) override;

protected:
  ErrorCode updateStopInfo(int waitStatus) override;
  void updateState() override;

protected:
  ErrorCode prepareSoftwareSingleStep(Address const &address) override;
};
}
}
}

#endif // !__DebugServer2_Target_Linux_Thread_h
