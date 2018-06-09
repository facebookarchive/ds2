//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Core/BreakpointManager.h"
#include "DebugServer2/Utils/Log.h"

namespace ds2 {

BreakpointManager::BreakpointManager(Target::ProcessBase *process)
    : _process(process) {}

BreakpointManager::~BreakpointManager() {
  // cannot call clear() here
}

void BreakpointManager::clear() { _sites.clear(); }

ErrorCode BreakpointManager::add(Address const &address, Lifetime lifetime,
                                 size_t size, Mode mode) {
  CHK(isValid(address, size, mode));

  if (size == 0) {
    size = chooseBreakpointSize();
  }

  auto it = _sites.find(address);
  if (it != _sites.end()) {
    if (it->second.mode != mode)
      return kErrorInvalidArgument;

    it->second.lifetime = static_cast<Lifetime>(it->second.lifetime | lifetime);
    if (lifetime == Lifetime::Permanent)
      ++it->second.refs;
  } else {
    Site &site = _sites[address];

    site.refs = 0;
    if (lifetime == Lifetime::Permanent)
      ++site.refs;
    site.address = address;
    site.lifetime = lifetime;
    site.mode = mode;
    site.size = size;

    // If the breakpoint manager is already in enabled state, enable
    // the newly added breakpoint too.
    if (enabled()) {
      return enableLocation(site);
    }
  }

  return kSuccess;
}

ErrorCode BreakpointManager::remove(Address const &address) {
  if (!address.valid())
    return kErrorInvalidArgument;

  auto it = _sites.find(address);
  if (it == _sites.end())
    return kErrorNotFound;

  // If we have some sort of temporary breakpoint, just remove it.
  if ((it->second.lifetime & Lifetime::Permanent) == Lifetime::None)
    goto do_remove;

  // If we have a permanent breakpoint, refs should be non-null. If refs is
  // still non-null after the decrement, do not remove the breakpoint and
  // return.
  DS2ASSERT(it->second.refs > 0);
  if (--it->second.refs > 0)
    return kSuccess;

  // If we had a breakpoint that only had lifetime Lifetime::Permanent and refs
  // is now 0, we need to remove it.
  if (it->second.lifetime == Lifetime::Permanent)
    goto do_remove;

  // Otherwise, the breakpoint had multiple lifetimes; unset
  // Lifetime::Permanent and return.
  it->second.lifetime = it->second.lifetime & ~Lifetime::Permanent;
  return kSuccess;

do_remove:
  DS2ASSERT(it->second.refs == 0);
  //
  // If the breakpoint manager is already in enabled state, disable
  // the newly removed breakpoint too.
  //
  ErrorCode error = kSuccess;
  if (enabled()) {
    error = disableLocation(it->second);
  }

  _sites.erase(it);
  return error;
}

bool BreakpointManager::has(Address const &address) const {
  if (!address.valid())
    return false;

  return (_sites.find(address) != _sites.end());
}

void BreakpointManager::enumerate(
    std::function<void(Site const &)> const &cb) const {
  for (auto const &it : _sites) {
    cb(it.second);
  }
}

void BreakpointManager::enable(Target::Thread *thread) {
  //
  // Both callbacks should be installed by child class before
  // enabling the breakpoint manager.
  //
  if (enabled(thread)) {
    DS2LOG(Warning, "double-enabling breakpoints");
  }

  enumerate([this, thread](Site const &site) { enableLocation(site, thread); });
}

void BreakpointManager::disable(Target::Thread *thread) {
  if (!enabled(thread)) {
    DS2LOG(Warning, "double-disabling breakpoints");
  }

  enumerate(
      [this, thread](Site const &site) { disableLocation(site, thread); });

  //
  // Remove temporary breakpoints.
  //
  auto it = _sites.begin();
  while (it != _sites.end()) {
    it->second.lifetime = it->second.lifetime & ~Lifetime::TemporaryOneShot;
    if (it->second.lifetime == Lifetime::None) {
      // refs should always be 0 unless we have a Lifetime::Permanent
      // breakpoint.
      DS2ASSERT(it->second.refs == 0);
      _sites.erase(it++);
    } else {
      it++;
    }
  }
}

bool BreakpointManager::hit(Address const &address, Site &site) {
  if (!address.valid())
    return false;

  auto it = _sites.find(address);
  if (it == _sites.end())
    return false;

  //
  // If this breakpoint's lifetime becomes 0, it will be erased after calling
  // BreakpointManager::disable
  //
  it->second.lifetime =
      static_cast<Lifetime>(it->second.lifetime & ~Lifetime::TemporaryUntilHit);

  site = it->second;
  return true;
}

ErrorCode BreakpointManager::isValid(Address const &address, size_t size,
                                     Mode mode) const {
  if (!address.valid()) {
    return kErrorInvalidArgument;
  }

  return kSuccess;
}
} // namespace ds2
