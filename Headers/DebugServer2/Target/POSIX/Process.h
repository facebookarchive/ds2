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

#include "DebugServer2/Host/POSIX/PTrace.h"
#include "DebugServer2/Host/ProcessSpawner.h"
#include "DebugServer2/Target/ProcessBase.h"

namespace ds2 {
namespace Target {
namespace POSIX {

class Process : public ds2::Target::ProcessBase {
protected:
  std::set<int> _passthruSignals;

protected:
  ErrorCode initialize(ProcessId pid, uint32_t flags) override;
  virtual ErrorCode attach(int waitStatus) = 0;

public:
  ErrorCode detach() override;
  ErrorCode interrupt() override;
  ErrorCode terminate() override;
  bool isAlive() const override;

public:
  ErrorCode readString(Address const &address, std::string &str, size_t length,
                       size_t *count = nullptr) override;
  ErrorCode readMemory(Address const &address, void *data, size_t length,
                       size_t *count = nullptr) override;
  ErrorCode writeMemory(Address const &address, void const *data, size_t length,
                        size_t *count = nullptr) override;

protected:
  int convertMemoryProtectionToPOSIX(uint32_t protection) const;
  uint32_t convertMemoryProtectionFromPOSIX(int POSIXProtection) const;

public:
  void resetSignalPass();
  void setSignalPass(int signo, bool set);

public:
  ErrorCode wait() override;

public:
  virtual Host::POSIX::PTrace &ptrace() const = 0;

public:
  static ds2::Target::Process *Create(Host::ProcessSpawner &spawner);
  static ds2::Target::Process *Attach(ProcessId pid);
};
} // namespace POSIX
} // namespace Target
} // namespace ds2
