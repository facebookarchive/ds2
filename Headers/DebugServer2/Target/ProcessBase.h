//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Target_ProcessBase_h
#define __DebugServer2_Target_ProcessBase_h

#include "DebugServer2/Target/ProcessDecl.h"
#include "DebugServer2/Target/ThreadBase.h"

namespace ds2 {

class BreakpointManager;
class WatchpointManager;

namespace Target {

class ProcessBase {
protected:
  enum { kFlagAttached = (1 << 0) };

protected:
  uint32_t _flags;
  ProcessId _pid;
  ProcessInfo _info;
  Address _loadBase;
  Address _entryPoint;
  ThreadBase::IdentityMap _threads;
  Thread *_currentThread;

public:
  struct SharedLibrary {
    bool main;
    std::string path;
    struct {
      uint64_t mapAddress;
      uint64_t baseAddress;
      uint64_t ldAddress;
    } svr4;
    std::vector<uint64_t> sections;
  };

protected:
  ProcessBase();

public:
  virtual ~ProcessBase();

public:
  inline ProcessId pid() const { return _pid; }

public:
  inline bool attached() const { return (_flags & kFlagAttached) != 0; }

public:
  inline Address const &loadBase() const { return _loadBase; }
  inline Address const &entryPoint() const { return _entryPoint; }

public:
  inline Thread *currentThread() const {
    return const_cast<ProcessBase *>(this)->_currentThread;
  }
  Thread *thread(ThreadId tid) const;

protected:
  virtual ErrorCode initialize(ProcessId pid, uint32_t flags);

public:
  virtual ErrorCode getInfo(ProcessInfo &info) = 0;

public: // ELF only
  virtual ErrorCode getAuxiliaryVector(std::string &auxv);
  virtual uint64_t getAuxiliaryVectorValue(uint64_t type);

protected:
  virtual void cleanup();

public:
  virtual ErrorCode attach(bool rettach = false) = 0;
  virtual ErrorCode detach() = 0;

public:
  virtual ErrorCode suspend() = 0;
  virtual ErrorCode
  resume(int signal = 0,
         std::set<Thread *> const &excluded = std::set<Thread *>()) = 0;

public:
  virtual ErrorCode interrupt() = 0;
  virtual ErrorCode terminate() = 0;
  virtual bool isAlive() const = 0;

public:
  virtual ErrorCode
  enumerateThreads(std::function<void(Thread *)> const &cb) const;

public:
  virtual ErrorCode readMemory(Address const &address, void *buffer,
                               size_t length, size_t *nread = nullptr) = 0;
  virtual ErrorCode writeMemory(Address const &address, void const *buffer,
                                size_t length, size_t *nwritten = nullptr) = 0;

public:
  ErrorCode readMemoryBuffer(Address const &address, size_t length,
                             std::string &buffer);
  ErrorCode writeMemoryBuffer(Address const &address, std::string const &buffer,
                              size_t *nwritten = nullptr);
  ErrorCode writeMemoryBuffer(Address const &address, std::string const &buffer,
                              size_t length, size_t *nwritten = nullptr);

public:
  virtual ErrorCode allocateMemory(size_t size, uint32_t protection,
                                   uint64_t *address) = 0;
  virtual ErrorCode deallocateMemory(uint64_t address, size_t size) = 0;

public:
  virtual ErrorCode getMemoryRegionInfo(Address const &address,
                                        MemoryRegionInfo &info) = 0;

public:
  virtual void getThreadIds(std::vector<ThreadId> &tids);

protected:
  virtual ErrorCode updateInfo() = 0;

public:
  virtual BreakpointManager *breakpointManager() const = 0;
  virtual WatchpointManager *watchpointManager() const = 0;

public:
  virtual bool isELFProcess() const = 0;

public:
  virtual bool isSingleStepSupported() const;

public:
  virtual void prepareForDetach();
  virtual ErrorCode beforeResume();
  virtual ErrorCode afterResume();

public:
  virtual Architecture::GDBDescriptor const *
  getGDBRegistersDescriptor() const = 0;
  virtual Architecture::LLDBDescriptor const *
  getLLDBRegistersDescriptor() const = 0;

protected:
  friend class ThreadBase;
  virtual void insert(ThreadBase *thread);
  virtual void remove(ThreadBase *thread);
  virtual void removeThread(ThreadId tid);
};
}
}

#endif // !__DebugServer2_Target_ProcessBase_h
