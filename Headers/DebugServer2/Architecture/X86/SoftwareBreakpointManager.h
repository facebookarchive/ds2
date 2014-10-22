//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Architecture_X86_SoftwareBreakpointManager_h
#define __DebugServer2_Architecture_X86_SoftwareBreakpointManager_h

#include "DebugServer2/BreakpointManager.h"

namespace ds2 {
namespace Architecture {
namespace X86 {

class SoftwareBreakpointManager : public BreakpointManager {
private:
  std::map<uint64_t, uint8_t> _insns;

public:
  SoftwareBreakpointManager(Target::Process *process);
  ~SoftwareBreakpointManager();

protected:
  virtual bool hit(Target::Thread *thread);

public:
  virtual void clear();

public:
  virtual ErrorCode add(Address const &address, Type type, size_t size);

protected:
  virtual void enableLocation(Site const &site);
  virtual void disableLocation(Site const &site);
};
}
}
}

#endif // !__DebugServer2_Architecture_X86_SoftwareBreakpointManager_h
