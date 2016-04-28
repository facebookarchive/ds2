//
// Copyright (c) 2014-present, Facebook, Inc.
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
  SoftwareBreakpointManager(Target::ProcessBase *process);
  ~SoftwareBreakpointManager() override;

protected:
  bool hit(Target::Thread *thread) override;

public:
  void clear() override;

public:
  ErrorCode add(Address const &address, Type type, size_t size,
                Mode mode) override;

protected:
  ErrorCode enableLocation(Site const &site) override;
  ErrorCode disableLocation(Site const &site) override;

protected:
  friend Target::ProcessBase;
};
}
}
}

#endif // !__DebugServer2_Architecture_X86_SoftwareBreakpointManager_h
