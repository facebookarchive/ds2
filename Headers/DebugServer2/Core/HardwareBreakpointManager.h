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

#include "DebugServer2/Core/BreakpointManager.h"

#include <unordered_set>

namespace ds2 {

class HardwareBreakpointManager : public BreakpointManager {
public:
  HardwareBreakpointManager(Target::ProcessBase *process);
  ~HardwareBreakpointManager() override;

public:
  ErrorCode add(Address const &address, Lifetime lifetime, size_t size,
                Mode mode) override;
  ErrorCode remove(Address const &address) override;

public:
  int hit(Target::Thread *thread, Site &site) override;

protected:
  ErrorCode enableLocation(Site const &site,
                           Target::Thread *thread = nullptr) override;
  virtual ErrorCode enableLocation(Site const &site, int idx,
                                   Target::Thread *thread);
  ErrorCode disableLocation(Site const &site,
                            Target::Thread *thread = nullptr) override;
  virtual ErrorCode disableLocation(int idx, Target::Thread *thread);

protected:
  bool enabled(Target::Thread *thread = nullptr) const override;

public:
  virtual size_t maxWatchpoints();

public:
  void enable(Target::Thread *thread = nullptr) override;
  void disable(Target::Thread *thread = nullptr) override;

protected:
  ErrorCode isValid(Address const &address, size_t size,
                    Mode mode) const override;
  size_t chooseBreakpointSize() const override;

protected:
  virtual int getAvailableLocation();

protected:
  virtual void
  enumerateThreads(Target::Thread *thread,
                   std::function<void(Target::Thread *t)> const &cb) const;

protected:
  ErrorCode readDebugRegisters(Target::Thread *thread,
                               std::vector<uint64_t> &regs) const;
  ErrorCode writeDebugRegisters(Target::Thread *thread,
                                std::vector<uint64_t> &regs) const;

protected:
  /*
   * TODO(asb) currently _locations is used in tandem with _sites, where it's
   * only purpose is to (as a vector) track the index of an address in debugRegs
   * (see sources/core/x86/HWBPManager.enableLocation, whilst paying attention
   * to the idx parameter).
   * It would be nice to delete _locations and store the index in _sites.
   * Currently the only reason we're doing that is because it isn't possible to
   * make _sites (a std::map fixed-size, to stay true to the fixed size
   * debugRegs on the platform).
   */
  std::vector<uint64_t> _locations;
  std::unordered_set<ThreadId> _enabled;

#if defined(ARCH_X86) || defined(ARCH_X86_64)
protected:
  virtual ErrorCode disableDebugCtrlReg(uint64_t &ctrlReg, int idx);
  virtual ErrorCode enableDebugCtrlReg(uint64_t &ctrlReg, int idx, Mode mode,
                                       int size);
#endif

public:
  virtual bool fillStopInfo(Target::Thread *thread,
                            StopInfo &stopInfo) override;
};
} // namespace ds2
