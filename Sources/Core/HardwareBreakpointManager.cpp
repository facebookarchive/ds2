//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Core/HardwareBreakpointManager.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Target/Thread.h"

#include <algorithm>

#define super ds2::BreakpointManager

namespace ds2 {

HardwareBreakpointManager::HardwareBreakpointManager(
    Target::ProcessBase *process)
    : super(process), _locations(maxWatchpoints(), 0) {}

HardwareBreakpointManager::~HardwareBreakpointManager() {}

ErrorCode HardwareBreakpointManager::add(Address const &address,
                                         Lifetime lifetime, size_t size,
                                         Mode mode) {
  if (_sites.size() >= maxWatchpoints()) {
    return kErrorInvalidArgument;
  }

  if (mode == kModeRead) {
    DS2LOG(Warning,
           "read-only watchpoints are unsupported, setting as read-write");
    mode = static_cast<Mode>(mode | kModeWrite);
  }

  return super::add(address, lifetime, size, mode);
}

ErrorCode HardwareBreakpointManager::remove(Address const &address) {
  auto loc = std::find(_locations.begin(), _locations.end(), address);
  if (loc != _locations.end()) {
    _locations[loc - _locations.begin()] = 0;
  }

  return super::remove(address);
}

ErrorCode HardwareBreakpointManager::enableLocation(Site const &site,
                                                    Target::Thread *thread) {
  ErrorCode error;
  int idx;

  auto loc = std::find(_locations.begin(), _locations.end(), site.address);
  if (loc == _locations.end()) {
    idx = getAvailableLocation();
    if (idx < 0) {
      return kErrorInvalidArgument;
    }
  } else {
    idx = loc - _locations.begin();
  }

  enumerateThreads(thread, [&](Target::Thread *t) {
    if (t->state() == Target::Thread::kStopped && !enabled(t)) {
      error = enableLocation(site, idx, t);
    }
  });

  if (error == kSuccess) {
    _locations[idx] = site.address;
  }
  return error;
}

ErrorCode HardwareBreakpointManager::disableLocation(Site const &site,
                                                     Target::Thread *thread) {
  ErrorCode error;

  auto loc = std::find(_locations.begin(), _locations.end(), site.address);
  if (loc == _locations.end()) {
    return kErrorInvalidArgument;
  }
  int idx = loc - _locations.begin();

  enumerateThreads(thread, [&](Target::Thread *t) {
    if (t->state() == Target::Thread::kStopped && enabled(t)) {
      error = disableLocation(idx, t);
    }
  });

  return error;
}

int HardwareBreakpointManager::getAvailableLocation() {
  DS2ASSERT(_locations.size() == maxWatchpoints());
  if (_sites.size() == maxWatchpoints()) {
    return -1;
  }

  auto it = std::find(_locations.begin(), _locations.end(), 0);
  DS2ASSERT(it != _locations.end());

  return (it - _locations.begin());
}

void HardwareBreakpointManager::enumerateThreads(
    Target::Thread *thread,
    std::function<void(Target::Thread *)> const &cb) const {
  std::set<Target::Thread *> threads;

  if (thread != nullptr) {
    threads.insert(thread);
  } else {
    _process->enumerateThreads([&](Target::Thread *t) { threads.insert(t); });
  }

  for (auto t : threads) {
    cb(t);
  }
}

void HardwareBreakpointManager::enable(Target::Thread *thread) {
  super::enable(thread);

  enumerateThreads(thread,
                   [this](Target::Thread *t) { _enabled.insert(t->tid()); });
}

void HardwareBreakpointManager::disable(Target::Thread *thread) {
  super::disable(thread);

  enumerateThreads(thread,
                   [this](Target::Thread *t) { _enabled.erase(t->tid()); });
}

bool HardwareBreakpointManager::enabled(Target::Thread *thread) const {
  bool isEnabled = true;

  enumerateThreads(thread, [this, &isEnabled](Target::Thread *t) {
    if (_enabled.count(t->tid()) == 0) {
      isEnabled = false;
    }
  });

  return isEnabled;
}

bool HardwareBreakpointManager::fillStopInfo(Target::Thread *thread,
                                             StopInfo &stopInfo) {
  BreakpointManager::Site site;
  int bpIdx = hit(thread, site);
  if (bpIdx < 0) {
    return false;
  }

  stopInfo.watchpointIndex = bpIdx;
  stopInfo.watchpointAddress = site.address;
  switch (static_cast<int>(site.mode)) {
  case BreakpointManager::kModeExec:
    stopInfo.reason = StopInfo::kReasonBreakpoint;
    break;
  case BreakpointManager::kModeWrite:
    stopInfo.reason = StopInfo::kReasonWriteWatchpoint;
    break;
  case BreakpointManager::kModeRead:
    stopInfo.reason = StopInfo::kReasonReadWatchpoint;
    break;
  case BreakpointManager::kModeRead | BreakpointManager::kModeWrite:
    stopInfo.reason = StopInfo::kReasonAccessWatchpoint;
    break;
  default:
    DS2BUG("invalid mode");
  }
  return true;
}
} // namespace ds2
