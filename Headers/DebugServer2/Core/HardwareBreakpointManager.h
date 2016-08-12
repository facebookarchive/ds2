//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_HardwareBreakpointManager_h
#define __DebugServer2_HardwareBreakpointManager_h

#include "DebugServer2/Core/BreakpointManager.h"

namespace ds2 {

class HardwareBreakpointManager : public BreakpointManager {
public:
  HardwareBreakpointManager(Target::ProcessBase *process);
  ~HardwareBreakpointManager() override;

public:
  ErrorCode add(Address const &address, Type type, size_t size,
                Mode mode) override;
  ErrorCode remove(Address const &address) override;

public:
  int hit(Target::Thread *thread, Site &site) override;

protected:
  ErrorCode enableLocation(Site const &site) override;
  virtual ErrorCode enableLocation(Site const &site, int idx,
                                   Target::Thread *thread);
  ErrorCode disableLocation(Site const &site) override;
  virtual ErrorCode disableLocation(int idx, Target::Thread *thread);

public:
  virtual size_t maxWatchpoints();

protected:
  ErrorCode isValid(Address const &address, size_t size,
                    Mode mode) const override;

protected:
  virtual int getAvailableLocation();

protected:
  std::vector<uint64_t> _locations;

#if defined(ARCH_X86) || defined(ARCH_X86_64)
protected:
  virtual ErrorCode disableDebugCtrlReg(uint32_t &ctrlReg, int idx);
  virtual ErrorCode enableDebugCtrlReg(uint32_t &ctrlReg, int idx, Mode mode,
                                       int size);
#endif
};
}

#endif // !__DebugServer2_HardwareBreakpointManager_h
