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

ErrorCode HardwareBreakpointManager::add(Address const &address, Type type,
                                         size_t size, Mode mode) {
  if (_sites.size() >= maxWatchpoints()) {
    return kErrorInvalidArgument;
  }

  if (mode == kModeRead) {
    DS2LOG(Warning,
           "read-only watchpoints are unsupported, setting as read-write");
    mode = static_cast<Mode>(mode | kModeWrite);
  }

  return super::add(address, type, size, mode);
}

ErrorCode HardwareBreakpointManager::remove(Address const &address) {
  auto loc = std::find(_locations.begin(), _locations.end(), address);
  if (loc != _locations.end()) {
    _locations[loc - _locations.begin()] = 0;
  }

  return super::remove(address);
}

ErrorCode HardwareBreakpointManager::enableLocation(Site const &site) {
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

  std::set<Target::Thread *> threads;
  _process->enumerateThreads([&](Target::Thread *thread) {
    if (thread->state() == Target::Thread::kStopped) {
      threads.insert(thread);
    }
  });

  for (auto thread : threads) {
    CHK(enableLocation(site, idx, thread));
  }

  _locations[idx] = site.address;
  return kSuccess;
}

ErrorCode HardwareBreakpointManager::disableLocation(Site const &site) {
  auto loc = std::find(_locations.begin(), _locations.end(), site.address);
  if (loc == _locations.end()) {
    return kErrorInvalidArgument;
  }
  int idx = loc - _locations.begin();

  std::set<Target::Thread *> threads;
  _process->enumerateThreads([&](Target::Thread *thread) {
    if (thread->state() == Target::Thread::kStopped) {
      threads.insert(thread);
    }
  });

  for (auto thread : threads) {
    CHK(disableLocation(idx, thread));
  }

  return kSuccess;
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
}
