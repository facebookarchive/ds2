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

#include "DebugServer2/Target/ProcessBase.h"

namespace ds2 {
namespace Target {
namespace Windows {

class Process : public ds2::Target::ProcessBase {
public:
  virtual ErrorCode initialize(ProcessId pid, uint32_t flags) {
    return ds2::kErrorUnsupported;
  }
  virtual ErrorCode attach(bool reattach = false) {
    return ds2::kErrorUnsupported;
  }
  virtual ErrorCode detach() { return ds2::kErrorUnsupported; }
  virtual ErrorCode interrupt() { return ds2::kErrorUnsupported; }
  virtual ErrorCode terminate(int signal = 0) { return ds2::kErrorUnsupported; }
  virtual bool isAlive() const;

public:
  virtual ErrorCode suspend() { return ds2::kErrorUnsupported; }
  virtual ErrorCode
  resume(int signal = 0,
         std::set<Thread *> const &excluded = std::set<Thread *>()) {
    return ds2::kErrorUnsupported;
  }

public:
  ErrorCode readMemory(Address const &address, void *data, size_t length,
                       size_t *nread = nullptr) {
    return ds2::kErrorUnsupported;
  }
  ErrorCode writeMemory(Address const &address, void const *data, size_t length,
                        size_t *nwritten = nullptr) {
    return ds2::kErrorUnsupported;
  }

public:
  virtual ErrorCode getMemoryRegionInfo(Address const &address,
                                        MemoryRegionInfo &info);

public:
  virtual ErrorCode allocateMemory(size_t size, uint32_t protection,
                                   uint64_t *address) {
    return ds2::kErrorUnsupported;
  }
  virtual ErrorCode deallocateMemory(uint64_t address, size_t size) {
    return ds2::kErrorUnsupported;
  }

public:
  void resetSignalPass() {}
  void setSignalPass(int signo, bool set) {}

public:
  virtual ErrorCode wait(int *status = nullptr, bool hang = true) {
    return ds2::kErrorUnsupported;
  }

public:
  virtual void ptrace() const;

public:
  static ds2::Target::Process *Create(int argc, char **argv) { return nullptr; }
  static ds2::Target::Process *
  Create(std::string const &path,
         StringCollection const &args = StringCollection()) {
    return nullptr;
  }
  static ds2::Target::Process *Attach(ProcessId pid) { return nullptr; }

public:
  virtual ErrorCode getSharedLibraryInfoAddress(Address &address) {
    return ds2::kErrorUnsupported;
  }
  virtual ErrorCode enumerateSharedLibraries(
      std::function<void(SharedLibrary const &)> const &cb) {
    return ds2::kErrorUnsupported;
  }
};
}
}
}

#endif // !__DebugServer2_Target_Windows_Process_h
