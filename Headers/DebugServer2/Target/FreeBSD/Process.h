//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Target_FreeBSD_Process_h
#define __DebugServer2_Target_FreeBSD_Process_h

#include "DebugServer2/Host/FreeBSD/PTrace.h"
#include "DebugServer2/Target/POSIX/ELFProcess.h"

namespace ds2 {
namespace Target {
namespace FreeBSD {

class Process : public ds2::Target::POSIX::ELFProcess {
protected:
  Host::FreeBSD::PTrace _ptrace;
  SoftwareBreakpointManager *_softwareBreakpointManager;
  HardwareBreakpointManager *_hardwareBreakpointManager;
  bool _terminated;

protected:
  friend class POSIX::Process;
  Process();

public:
  virtual ~Process();

protected:
  virtual ErrorCode initialize(ProcessId pid, uint32_t flags);
  ErrorCode attach(int waitStatus);

public:
  virtual ErrorCode terminate();

public:
  virtual ErrorCode suspend();

public:
  virtual ErrorCode getMemoryRegionInfo(Address const &address,
                                        MemoryRegionInfo &info);

public:
  virtual ErrorCode allocateMemory(size_t size, uint32_t protection,
                                   uint64_t *address);
  virtual ErrorCode deallocateMemory(uint64_t address, size_t size);

public:
  virtual ErrorCode wait(int *status = nullptr);

public:
  virtual Host::POSIX::PTrace &ptrace() const;

protected:
  virtual ErrorCode updateInfo();
  virtual ErrorCode updateAuxiliaryVector();
  virtual ErrorCode enumerateAuxiliaryVector(
      std::function<
          void(Support::ELFSupport::AuxiliaryVectorEntry const &)> const &cb);

public:
  virtual bool isSingleStepSupported() const;

public:
  virtual SoftwareBreakpointManager *softwareBreakpointManager() const;
  virtual HardwareBreakpointManager *hardwareBreakpointManager() const;

protected:
  friend class Thread;
  ErrorCode readCPUState(ThreadId tid, Architecture::CPUState &state,
                         uint32_t flags = 0);
  ErrorCode writeCPUState(ThreadId tid, Architecture::CPUState const &state,
                          uint32_t flags = 0);

public:
  virtual ErrorCode readString(Address const &address, std::string &str,
                               size_t length, size_t *nread = nullptr);
  virtual ErrorCode readMemory(Address const &address, void *data,
                               size_t length, size_t *count = nullptr);
  virtual ErrorCode writeMemory(Address const &address, void const *data,
                                size_t length, size_t *count = nullptr);

public:
  virtual Architecture::GDBDescriptor const *getGDBRegistersDescriptor() const;
  virtual Architecture::LLDBDescriptor const *
  getLLDBRegistersDescriptor() const;
};
}
}
}

#endif // !__DebugServer2_Target_FreeBSD_Process_h
