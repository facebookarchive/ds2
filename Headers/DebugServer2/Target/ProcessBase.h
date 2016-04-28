//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Target_ProcessBase_h
#define __DebugServer2_Target_ProcessBase_h

#include "DebugServer2/HardwareBreakpointManager.h"
#include "DebugServer2/SoftwareBreakpointManager.h"
#include "DebugServer2/Target/ProcessDecl.h"
#include "DebugServer2/Target/ThreadBase.h"

#include <functional>
#include <set>

namespace ds2 {
namespace Target {

class ProcessBase {
public:
  enum { kFlagNewProcess = 0, kFlagAttachedProcess = (1 << 0) };
  typedef std::map<ThreadId, Thread *> IdentityMap;

protected:
  bool _terminated;
  uint32_t _flags;
  ProcessId _pid;
  ProcessInfo _info;
  Address _loadBase;
  Address _entryPoint;
  IdentityMap _threads;
  Thread *_currentThread;
  mutable SoftwareBreakpointManager *_softwareBreakpointManager;
  mutable HardwareBreakpointManager *_hardwareBreakpointManager;

protected:
  ProcessBase();
  virtual ~ProcessBase();

public:
  inline ProcessId pid() const { return _pid; }

public:
  inline bool attached() const { return (_flags & kFlagAttachedProcess) != 0; }

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
  virtual ErrorCode getInfo(ProcessInfo &info);

public: // ELF only
  virtual ErrorCode getAuxiliaryVector(std::string &auxv);
  virtual uint64_t getAuxiliaryVectorValue(uint64_t type);

protected:
  virtual void cleanup();

public:
  virtual ErrorCode detach() = 0;

public:
  virtual ErrorCode suspend();
  virtual ErrorCode
  resume(int signal = 0,
         std::set<Thread *> const &excluded = std::set<Thread *>());

public:
  virtual ErrorCode interrupt() = 0;
  virtual ErrorCode terminate() = 0;
  virtual bool isAlive() const = 0;

public:
  virtual ErrorCode
  enumerateThreads(std::function<void(Thread *)> const &cb) const;

public:
  virtual ErrorCode readString(Address const &address, std::string &str,
                               size_t length, size_t *nread = nullptr) = 0;
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
  virtual ErrorCode wait() = 0;

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
  virtual SoftwareBreakpointManager *softwareBreakpointManager() const final;
  virtual HardwareBreakpointManager *hardwareBreakpointManager() const final;

public:
  virtual bool isELFProcess() const = 0;

public:
  virtual void prepareForDetach();
  virtual ErrorCode beforeResume();
  virtual ErrorCode afterResume();

public:
  virtual int getMaxBreakpoints() const { return 0; }
  virtual int getMaxWatchpoints() const { return 0; }
  virtual int getMaxWatchpointSize() const { return 0; }

public:
  // There shouldn't be any reason for these to be overridden.
  virtual Architecture::GDBDescriptor const *
  getGDBRegistersDescriptor() const final;
  virtual Architecture::LLDBDescriptor const *
  getLLDBRegistersDescriptor() const final;

protected:
  friend class ThreadBase;
  virtual void insert(ThreadBase *thread);
  virtual void remove(ThreadBase *thread);
  virtual void removeThread(ThreadId tid);
};
}
}

#endif // !__DebugServer2_Target_ProcessBase_h
