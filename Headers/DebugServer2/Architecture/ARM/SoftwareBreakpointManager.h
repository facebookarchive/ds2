//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Architecture_ARM_SoftwareBreakpointManager_h
#define __DebugServer2_Architecture_ARM_SoftwareBreakpointManager_h

#include "DebugServer2/BreakpointManager.h"

namespace ds2 {
namespace Architecture {
namespace ARM {

class SoftwareBreakpointManager : public BreakpointManager {
private:
  std::map<uint64_t, std::string> _insns;

public:
  SoftwareBreakpointManager(Target::Process *process);
  ~SoftwareBreakpointManager();

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
  virtual bool hit(Target::Thread *thread);

protected:
  virtual void getOpcode(uint32_t type, std::string &opcode) const;

protected:
  virtual void enableLocation(Site const &site);
  virtual void disableLocation(Site const &site);
};
}
}
}

#endif // !__DebugServer2_Architecture_ARM_SoftwareBreakpointManager_h
