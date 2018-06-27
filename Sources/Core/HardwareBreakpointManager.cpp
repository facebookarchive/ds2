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
    : super(process), _locations(maxWatchpoints()) {}

HardwareBreakpointManager::~HardwareBreakpointManager() {}

ErrorCode HardwareBreakpointManager::add(Address const &address,
                                         Lifetime lifetime, size_t size,
                                         Mode mode) {
  if (_sites.size() >= maxWatchpoints()) {
    return kErrorInvalidArgument;
  }

  // Readonly hardware watchpoints aren't natively implemented, so they
  // are implemened in software instead.
  if (mode == kModeRead)
    mode = static_cast<Mode>(mode | kModeWrite);

  return super::add(address, lifetime, size, mode);
}

ErrorCode HardwareBreakpointManager::remove(Address const &address) {
  auto loc = std::find(_locations.begin(), _locations.end(), address);
  if (loc != _locations.end()) {
    _locations.erase(loc);
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
    CHK(storeNewValueAtAddress(site.address));
  }
  return error;
}

ErrorCode HardwareBreakpointManager::disableLocation(Site const &site,
                                                     Target::Thread *thread) {
  ErrorCode error;

  auto loc = std::find(_locations.begin(), _locations.end(), site.address);
  if (loc == _locations.end()) {
    return kErrorInvalidArgument;
  } else {
    int idx = loc - _locations.begin();

    enumerateThreads(thread, [&](Target::Thread *t) {
      if (t->state() == Target::Thread::kStopped && enabled(t)) {
        error = disableLocation(idx, t);
      }
    });

    return error;
  }
}

inline bool
HardwareBreakpointManager::softwareImplementationOfReadonlyWatchpoints(
    Address const &address, Site &site) {
  if (site.mode == kModeRead && wasWritten(address)) {
      return false;
  } else {
    return super::hit(address, site);
  }
}

ErrorCode
HardwareBreakpointManager::storeNewValueAtAddress(Address const &address) {
  uint64_t value;
  CHK(_process->readMemory(address, &value, sizeof(value)));

  try {
    // store the new value. this will throw if _sites[address.value()] == null
    _sites.at(address.value()).memoryValue = value;
    return kSuccess;
  } catch (const std::out_of_range &) {
    return kErrorInvalidAddress;
  }
}

bool HardwareBreakpointManager::hit(Address const &address, Site &site) {
  auto ret = softwareImplementationOfReadonlyWatchpoints(address, site);
  DS2ASSERT(storeNewValueAtAddress(address) == kSuccess);

  return ret;
}

int HardwareBreakpointManager::getAvailableLocation() {
  DS2ASSERT(_locations.size() == maxWatchpoints());
  if (_sites.size() == maxWatchpoints()) {
    return -1;
  }

  auto it = std::find(_locations.begin(), _locations.end(), 0);
  DS2ASSERT(it != _locations.end());

  return it - _locations.begin();
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
