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

#include "DebugServer2/Host/Linux/PTrace.h"
#include "DebugServer2/Target/POSIX/ELFProcess.h"

namespace ds2 {
namespace Target {
namespace Linux {

class Process : public POSIX::ELFProcess {
protected:
  Host::Linux::PTrace _ptrace;

protected:
  ErrorCode attach(int waitStatus) override;

public:
  ErrorCode terminate() override;
  bool isAlive() const override;

public:
  ErrorCode getMemoryRegionInfo(Address const &address,
                                MemoryRegionInfo &info) override;

protected:
  ErrorCode executeCode(ByteVector const &codestr, uint64_t &result);

public:
  ErrorCode readMemory(Address const &address, void *data, size_t length,
                       size_t *count = nullptr) override;
  ErrorCode writeMemory(Address const &address, void const *data, size_t length,
                        size_t *count = nullptr) override;

public:
  ErrorCode allocateMemory(size_t size, uint32_t protection,
                           uint64_t *address) override;
  ErrorCode deallocateMemory(uint64_t address, size_t size) override;

protected:
  ErrorCode checkMemoryErrorCode(uint64_t address);

public:
  ErrorCode wait() override;

public:
  Host::Linux::PTrace &ptrace() const override;

protected:
  ErrorCode updateInfo() override;
  ErrorCode updateAuxiliaryVector() override;

protected:
  friend class Thread;
  ErrorCode readCPUState(ThreadId tid, Architecture::CPUState &state,
                         uint32_t flags = 0);
  ErrorCode writeCPUState(ThreadId tid, Architecture::CPUState const &state,
                          uint32_t flags = 0);

#if defined(ARCH_ARM)
public:
  int getMaxBreakpoints() const override;
  int getMaxWatchpoints() const override;
  int getMaxWatchpointSize() const override;
#endif

protected:
  friend class POSIX::Process;
};
} // namespace Linux
} // namespace Target
} // namespace ds2
