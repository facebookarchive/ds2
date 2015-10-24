//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Target_FreeBSD_Process_h
#define __DebugServer2_Target_FreeBSD_Process_h

#include "DebugServer2/Target/POSIX/ELFProcess.h"
#include "DebugServer2/Host/FreeBSD/PTrace.h"

namespace ds2 {
namespace Target {
namespace FreeBSD {

class Process : public ds2::Target::POSIX::ELFProcess {
protected:
  Host::FreeBSD::PTrace _ptrace;
  BreakpointManager *_breakpointManager;
  WatchpointManager *_watchpointManager;
  bool _terminated;

protected:
  friend class POSIX::Process;
  Process();

public:
  virtual ~Process();

public:
  virtual ErrorCode initialize(ProcessId pid, uint32_t flags);

public:
  virtual ErrorCode attach(bool reattach = false);

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
  virtual ErrorCode wait(int *status = nullptr, bool hang = true);

public:
  virtual Host::POSIX::PTrace &ptrace() const;

protected:
  virtual ErrorCode updateInfo();
  virtual ErrorCode updateAuxiliaryVector();
  virtual ErrorCode enumerateAuxiliaryVector(std::function<
          void(Support::ELFSupport::AuxiliaryVectorEntry const &)> const &cb);
public:
  virtual bool isSingleStepSupported() const;

public:
  virtual BreakpointManager *breakpointManager() const;
  virtual WatchpointManager *watchpointManager() const;

protected:
  friend class Thread;
  ErrorCode readCPUState(ThreadId tid, Architecture::CPUState &state,
                         uint32_t flags = 0);
  ErrorCode writeCPUState(ThreadId tid, Architecture::CPUState const &state,
                          uint32_t flags = 0);

protected:
  ErrorCode attach(int waitStatus);

public:
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
