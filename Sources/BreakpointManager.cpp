//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/BreakpointManager.h"
#include "DebugServer2/Log.h"

namespace ds2 {

BreakpointManager::BreakpointManager(Target::Process *process)
    : _enabled(false), _process(process) {}

BreakpointManager::~BreakpointManager() {
  // cannot call clear() here
}

void BreakpointManager::clear() { _sites.clear(); }

ErrorCode BreakpointManager::add(Address const &address, Type type,
                                 size_t size) {
  if (!address.valid())
    return kErrorInvalidArgument;

  auto it = _sites.find(address);
  if (it != _sites.end()) {
    //
    // Upgrade only to Permanent from Temporary
    // or from OneShot to UntilHit.
    //
    if ((type == kTypePermanent && type != it->second.type) ||
        (type == kTypeTemporaryUntilHit &&
         it->second.type == kTypeTemporaryOneShot)) {
      it->second.type = type;
    } else if (it->second.type == kTypePermanent) {
      //
      // Increment reference only if it's permanent and hasn't
      // been newly promoted to it.
      //
      it->second.refs++;
    }
  } else {
    Site &site = _sites[address];

    site.refs = 1;
    site.address = address;
    site.type = type;
    site.size = size;

    //
    // If the breakpoint manager is already in enabled state, enable
    // the newly added breakpoint too.
    //
    if (_enabled)
      enableLocation(site);
  }

  return kSuccess;
}

ErrorCode BreakpointManager::remove(Address const &address) {
  if (!address.valid())
    return kErrorInvalidArgument;

  auto it = _sites.find(address);
  if (it == _sites.end())
    return kErrorNotFound;

  if (--it->second.refs == 0) {
    //
    // If the breakpoint manager is already in enabled state, disable
    // the newly removed breakpoint too.
    //
    if (_enabled)
      disableLocation(it->second);

    _sites.erase(it);
  }

  return kSuccess;
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

void BreakpointManager::enable() {
  //
  // Both callbacks should be installed by child class before
  // enabling the breakpoint manager.
  //
  if (_enabled)
    DS2LOG(BPManager, Warning, "double-enabling breakpoints");
  _enabled = true;

  enumerate([this](Site const &site) { enableLocation(site); });
}

void BreakpointManager::disable() {
  if (!_enabled)
    DS2LOG(BPManager, Warning, "double-disabling breakpoints");
  _enabled = false;

  enumerate([this](Site const &site) { disableLocation(site); });

  //
  // Remove temporary breakpoints.
  //
  auto it = _sites.begin();
  while (it != _sites.end()) {
    if (it->second.type == kTypeTemporaryOneShot) {
      _sites.erase(it++);
    } else {
      ++it;
    }
  }
}

bool BreakpointManager::hit(Address const &address) {
  if (!address.valid())
    return false;

  auto it = _sites.find(address);
  if (it == _sites.end())
    return false;

  if (it->second.type != kTypePermanent && --it->second.refs == 0)
    _sites.erase(it);

  return true;
}
}