//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#if defined(__linux__)
#include "DebugServer2/Host/Linux/ExtraSyscalls.h"
#endif
#include "DebugServer2/Host/POSIX/AsyncProcessWaiter.h"

#if defined(__linux__)
#define DEFAULT_WAIT_FLAGS (__WALL | __WCLONE)
#else
#define DEFAULT_WAIT_FLAGS (0)
#endif

using ds2::Host::POSIX::AsyncProcessWaiter;

AsyncProcessWaiter::AsyncProcessWaiter() {}

AsyncProcessWaiter &AsyncProcessWaiter::Instance() {
  static AsyncProcessWaiter sAsyncWaiter;
  sAsyncWaiter.start();
  return sAsyncWaiter;
}

bool AsyncProcessWaiter::wait(ProcessId const &pid, ProcessId &wpid,
                              int &status, struct rusage &ru, bool hang) {
  if (pid <= 0)
    return false;

  std::set<ProcessId> pids;
  pids.insert(pid);
  return wait(pids, wpid, status, ru, hang);
}

bool AsyncProcessWaiter::wait(std::set<ProcessId> const &pids, ProcessId &wpid,
                              int &status, struct rusage &ru, bool hang) {
  if (pids.empty())
    return false;

  std::lock_guard<std::mutex> _(_lock);
  std::unique_lock<std::mutex> lock(_lock);

  for (;;) {
    for (auto pid : pids) {
      auto it = _events.find(pid);
      if (it != _events.end()) {
        wpid = it->second.pid;
        status = it->second.status;
        ru = it->second.ru;
        _events.erase(it);
        return true;
      }
    }

    if (!hang)
      break;

    _cv.wait(lock);
  }

  return false;
}

void AsyncProcessWaiter::start() {
  _thread = std::move(std::thread(&AsyncProcessWaiter::run, this));
}

void AsyncProcessWaiter::run() {
  for (;;) {
    Event event;

    pid_t pid = ::wait4(-1, &event.status, DEFAULT_WAIT_FLAGS, &event.ru);
    if (pid < 0)
      break;

    event.pid = pid;

    _lock.lock();
    _events.insert(std::make_pair(event.pid, event));
    _cv.notify_one();
    _lock.unlock();
  }
}
