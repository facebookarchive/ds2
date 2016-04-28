//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_BreakpointManager_h
#define __DebugServer2_BreakpointManager_h

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
    int32_t refs;

  public:
    Address address;
    Type type;
    Mode mode;
    size_t size;
  };

  // Address->Site map
  typedef std::map<uint64_t, Site> SiteMap;

protected:
  SiteMap _sites;
  bool _enabled;

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
  friend Target::Process;
  friend Target::ProcessBase;

protected:
  virtual bool hit(Address const &address);
  virtual bool hit(Target::Thread *thread) = 0;

protected:
  virtual void enable();
  virtual void disable();
  virtual ErrorCode enableLocation(Site const &site) = 0;
  virtual ErrorCode disableLocation(Site const &site) = 0;
};
}

#endif // !__DebugServer2_BreakpointManager_h
