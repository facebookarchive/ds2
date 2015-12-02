//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Host_POSIX_AsyncProcessWaiter_h
#define __DebugServer2_Host_POSIX_AsyncProcessWaiter_h

#include "DebugServer2/Types.h"

#include <condition_variable>
#include <map>
#include <mutex>
#include <set>
#include <sys/resource.h>
#include <sys/wait.h>
#include <thread>

namespace ds2 {
namespace Host {
namespace POSIX {

class AsyncProcessWaiter {
private:
  struct Event {
    ProcessId pid;
    int status;
  };

  typedef std::map<ProcessId, Event> EventsMap;

private:
  std::thread _thread;
  std::mutex _lock;
  std::condition_variable _cv;
  EventsMap _events;

public:
protected:
  AsyncProcessWaiter();

public:
  static AsyncProcessWaiter &Instance();

public:
  bool wait(std::set<ProcessId> const &pids, ProcessId &wpid, int &status,
            bool hang);
  bool wait(ProcessId const &pid, ProcessId &wpid, int &status, bool hang);

private:
  void start();
  void run();
};
}
}
}

#endif // !__DebugServer2_Host_POSIX_AsyncProcessWaiter_h
