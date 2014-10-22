//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_BreakpointManager_h
#define __DebugServer2_BreakpointManager_h

#include "DebugServer2/Target/Process.h"

namespace ds2 {

class BreakpointManager {
public:
  enum Type {
    kTypePermanent = 0,
    kTypeTemporaryOneShot = (1 << 0),
    kTypeTemporaryUntilHit = (1 << 1),
  };

public:
  struct Site {
  protected:
    friend class BreakpointManager;
    int32_t refs;

  public:
    Address address;
    Type type;
    size_t size;
  };

  // Address->Site map
  typedef std::map<uint64_t, Site> SiteMap;

protected:
  SiteMap _sites;

protected:
  bool _enabled;

protected:
  Target::Process *_process;

protected:
  BreakpointManager(Target::Process *process);

public:
  virtual ~BreakpointManager();

public:
  virtual void clear();

public:
  virtual ErrorCode add(Address const &address, Type type, size_t size);
  virtual ErrorCode remove(Address const &address);

public:
  virtual bool has(Address const &address) const;

public:
  virtual void enumerate(std::function<void(Site const &)> const &cb) const;

protected:
  friend Target::Process;
  friend Target::ProcessBase;

protected:
  virtual bool hit(Address const &address);
  virtual bool hit(Target::Thread *thread) = 0;

protected:
  virtual void enable();
  virtual void disable();
  virtual void enableLocation(Site const &site) = 0;
  virtual void disableLocation(Site const &site) = 0;
};
}

#endif // !__DebugServer2_BreakpointManager_h
