//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Target_POSIX_Thread_h
#define __DebugServer2_Target_POSIX_Thread_h

#include "DebugServer2/Target/ThreadBase.h"

namespace ds2 {
namespace Target {
namespace POSIX {

class Thread : public ds2::Target::ThreadBase {
protected:
  Thread(ds2::Target::Process *process, ThreadId tid);

public:
  ErrorCode terminate() override;
  ErrorCode suspend() override;

public:
  ErrorCode step(int signal = 0, Address const &address = Address()) override;
  ErrorCode resume(int signal = 0, Address const &address = Address()) override;

protected:
  virtual ErrorCode updateStopInfo(int waitStatus);
};
}
}
}

#endif // !__DebugServer2_Target_POSIX_Thread_h
