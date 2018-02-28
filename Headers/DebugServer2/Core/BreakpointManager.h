//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#pragma once

#include "DebugServer2/Target/ProcessDecl.h"

#include <functional>

namespace ds2 {

class BreakpointManager {
public:
  enum Type : unsigned int {
    kTypePermanent = (1 << 0),
    kTypeTemporaryOneShot = (1 << 1),
    kTypeTemporaryUntilHit = (1 << 2),
  };

  enum Mode {
    kModeExec = (1 << 0),
    kModeRead = (1 << 1),
    kModeWrite = (1 << 2),
  };

public:
  struct Site {
  protected:
    friend class BreakpointManager;
    friend class SoftwareBreakpointManager;
    int32_t refs;

  public:
    Address address;
    Type type;
    Mode mode;
    size_t size;

  public:
    bool operator==(Site const &other) const {
      return (address == other.address) && (type == other.type) &&
             (mode == other.mode) && (size == other.size);
    }
  };

  // Address->Site map
  typedef std::map<uint64_t, Site> SiteMap;

protected:
  SiteMap _sites;

protected:
  Target::ProcessBase *_process;

protected:
  BreakpointManager(Target::ProcessBase *process);

public:
  virtual ~BreakpointManager();

public:
  virtual void clear();

public:
  virtual ErrorCode add(Address const &address, Type type, size_t size,
                        Mode mode);
  virtual ErrorCode remove(Address const &address);

public:
  virtual bool has(Address const &address) const;

public:
  virtual void enumerate(std::function<void(Site const &)> const &cb) const;

protected:
  virtual ErrorCode isValid(Address const &address, size_t size,
                            Mode mode) const;

protected:
  friend Target::ProcessBase;

protected:
  virtual bool hit(Address const &address, Site &site);

public:
  // Returns the hardware index of the breakpoint, if applicable.
  // If not hit, returns a negative integer
  virtual int hit(Target::Thread *thread, Site &site) = 0;

protected:
  virtual ErrorCode enableLocation(Site const &site,
                                   Target::Thread *thread = nullptr) = 0;
  virtual ErrorCode disableLocation(Site const &site,
                                    Target::Thread *thread = nullptr) = 0;

public:
  virtual bool fillStopInfo(Target::Thread *thread, StopInfo &stopInfo) = 0;
};
} // namespace ds2
