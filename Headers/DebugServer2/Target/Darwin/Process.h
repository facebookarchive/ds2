//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Target_Darwin_Process_h
#define __DebugServer2_Target_Darwin_Process_h

#include "DebugServer2/Host/Darwin/PTrace.h"
#include "DebugServer2/Target/Darwin/MachOProcess.h"

namespace ds2 {
namespace Target {
namespace Darwin {

class Process : public MachOProcess {
protected:
  Host::Darwin::PTrace _ptrace;
  SoftwareBreakpointManager *_softwareBreakpointManager;
  HardwareBreakpointManager *_hardwareBreakpointManager;
  bool _terminated;

protected:
  friend class POSIX::Process;
  Process();

public:
  ~Process() override;

protected:
  ErrorCode initialize(ProcessId pid, uint32_t flags) override;
  ErrorCode attach(int waitStatus);

public:
  ErrorCode terminate() override;

public:
  ErrorCode suspend() override;

public:
  ErrorCode getMemoryRegionInfo(Address const &address,
                                MemoryRegionInfo &info) override;

public:
  ErrorCode allocateMemory(size_t size, uint32_t protection,
                           uint64_t *address) override;
  ErrorCode deallocateMemory(uint64_t address, size_t size) override;

public:
  ErrorCode wait() override;

public:
  Host::POSIX::PTrace &ptrace() const override;

protected:
  ErrorCode updateInfo() override;
  ErrorCode updateAuxiliaryVector() override;

public:
  bool isSingleStepSupported() const override;

public:
  SoftwareBreakpointManager *softwareBreakpointManager() const override;
  HardwareBreakpointManager *hardwareBreakpointManager() const override;

protected:
  friend class Thread;
  ErrorCode readCPUState(ThreadId tid, Architecture::CPUState &state,
                         uint32_t flags = 0);
  ErrorCode writeCPUState(ThreadId tid, Architecture::CPUState const &state,
                          uint32_t flags = 0);

public:
  ErrorCode readString(Address const &address, std::string &str, size_t length,
                       size_t *nread = nullptr) override;
  ErrorCode readMemory(Address const &address, void *data, size_t length,
                       size_t *count = nullptr) override;
  ErrorCode writeMemory(Address const &address, void const *data, size_t length,
                        size_t *count = nullptr) override;

public:
  Architecture::GDBDescriptor const *getGDBRegistersDescriptor() const override;
  Architecture::LLDBDescriptor const *
  getLLDBRegistersDescriptor() const override;
};
}
}
}

#endif // !__DebugServer2_Target_Darwin_Process_h
