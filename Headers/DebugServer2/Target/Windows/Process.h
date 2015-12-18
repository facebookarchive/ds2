//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Target_Windows_Process_h
#define __DebugServer2_Target_Windows_Process_h

#include "DebugServer2/Host/ProcessSpawner.h"
#include "DebugServer2/Target/ProcessBase.h"

namespace ds2 {
namespace Target {
namespace Windows {

class Process : public Target::ProcessBase {
protected:
  HANDLE _handle;
  BreakpointManager *_breakpointManager;
  bool _terminated;

protected:
  Process();

public:
  virtual ~Process();

public:
  inline HANDLE handle() const { return _handle; }

protected:
  virtual ErrorCode initialize(ProcessId pid, HANDLE handle, ThreadId tid,
                               HANDLE threadHandle, uint32_t flags);

public:
  virtual ErrorCode attach(bool reattach = false);
  virtual ErrorCode attach(ProcessId pid);
  virtual ErrorCode detach();

public:
  virtual ErrorCode interrupt();
  virtual ErrorCode terminate();
  virtual bool isAlive() const;

public:
  virtual ErrorCode suspend() { return kErrorUnsupported; }
  virtual ErrorCode
  resume(int signal = 0,
         std::set<Thread *> const &excluded = std::set<Thread *>());

public:
  ErrorCode readString(Address const &address, std::string &str, size_t length,
                       size_t *nread = nullptr);
  ErrorCode readMemory(Address const &address, void *data, size_t length,
                       size_t *nread = nullptr);
  ErrorCode writeMemory(Address const &address, void const *data, size_t length,
                        size_t *nwritten = nullptr);

public:
  virtual ErrorCode getMemoryRegionInfo(Address const &address,
                                        MemoryRegionInfo &info) {
    return kErrorUnsupported;
  }

public:
  virtual ErrorCode updateInfo();

public:
  virtual BreakpointManager *breakpointManager() const;
  virtual WatchpointManager *watchpointManager() const { return nullptr; }

public:
  virtual bool isELFProcess() const { return false; }

public:
  virtual ErrorCode allocateMemory(size_t size, uint32_t protection,
                                   uint64_t *address);
  virtual ErrorCode deallocateMemory(uint64_t address, size_t size);

public:
  void resetSignalPass() {}
  void setSignalPass(int signo, bool set) {}

public:
  virtual ErrorCode wait(int *status = nullptr, bool hang = true);

public:
  static Target::Process *Create(Host::ProcessSpawner &spawner);
  static Target::Process *Attach(ProcessId pid);

public:
  virtual ErrorCode getSharedLibraryInfoAddress(Address &address) {
    return kErrorUnsupported;
  }
  virtual ErrorCode enumerateSharedLibraries(
      std::function<void(SharedLibraryInfo const &)> const &cb);

public:
  virtual Architecture::GDBDescriptor const *getGDBRegistersDescriptor() const;
  virtual Architecture::LLDBDescriptor const *
  getLLDBRegistersDescriptor() const;
};
}
}
}

#endif // !__DebugServer2_Target_Windows_Process_h
