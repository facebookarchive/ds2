//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/ProcessBase.h"
#include "DebugServer2/Architecture/CPUState.h"
#include "DebugServer2/Core/SoftwareBreakpointManager.h"
#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Utils/Log.h"
#include "DebugServer2/Utils/Stringify.h"

#include <list>

using ds2::Utils::Stringify;

namespace ds2 {
namespace Target {

ProcessBase::ProcessBase()
    : _terminated(false), _flags(0), _pid(kAnyProcessId), _loadBase(),
      _entryPoint(), _currentThread(nullptr) {}

ProcessBase::~ProcessBase() {
  for (auto thread : _threads) {
    delete thread.second;
  }
  _threads.clear();
  _currentThread = nullptr;
}

ErrorCode ProcessBase::getInfo(ProcessInfo &info) {
  ErrorCode error = updateInfo();
  if (error == kSuccess || error == kErrorAlreadyExist) {
    info = _info;
    error = kSuccess;
  }
  return error;
}

// This is a utility function for detach.
void ProcessBase::cleanup() {
  std::set<Thread *> threads;
  for (auto thread : _threads) {
    threads.insert(thread.second);
  }
  for (auto thread : threads) {
    removeThread(thread->tid());
  }
  _threads.clear();
  _currentThread = nullptr;
}

ErrorCode ProcessBase::initialize(ProcessId pid, uint32_t flags) {
  if (_pid != kAnyProcessId) {
    return kErrorAlreadyExist;
  }

  _pid = pid;
  _flags = flags;

  //
  // Update process information immediatly.
  //
  ErrorCode error = updateInfo();
  if (error != kSuccess) {
    _pid = kAnyProcessId;
    _flags = 0;
  }

  return kSuccess;
}

ErrorCode ProcessBase::suspend() {
  std::set<Thread *> threads;
  enumerateThreads([&](Thread *thread) { threads.insert(thread); });

  for (auto thread : threads) {
    switch (thread->state()) {
    case Thread::kInvalid:
      DS2BUG("trying to suspend tid %" PRI_PID " in state %s", thread->tid(),
             Stringify::ThreadState(thread->state()));
      break;

    case Thread::kStepped:
    case Thread::kStopped:
    case Thread::kTerminated:
      DS2LOG(Debug, "not suspending tid %" PRI_PID ", already in state %s",
             thread->tid(), Stringify::ThreadState(thread->state()));
      if (thread->state() == Thread::kTerminated) {
        removeThread(thread->tid());
      }
      break;

    case Thread::kRunning: {
      DS2LOG(Debug, "suspending tid %" PRI_PID, thread->tid());
      ErrorCode error = thread->suspend();
      switch (error) {
      case kSuccess:
        break;

      case kErrorProcessNotFound:
        DS2LOG(Debug,
               "tried to suspended tid %" PRI_PID " which is already dead",
               thread->tid());
        removeThread(thread->tid());
        return error;

      default:
        DS2LOG(Warning, "failed suspending tid %" PRI_PID ", error=%s",
               thread->tid(), Stringify::Error(error));
        return error;
      }
    } break;
    }
  }

  return kSuccess;
}

ErrorCode ProcessBase::resume(int signal, std::set<Thread *> const &excluded) {
  enumerateThreads([&](Thread *thread) {
    if (excluded.find(thread) != excluded.end())
      return;

    switch (thread->state()) {
    case Thread::kInvalid:
    case Thread::kTerminated:
      DS2BUG("trying to resume tid %" PRI_PID " in state %s", thread->tid(),
             Stringify::ThreadState(thread->state()));
      break;

    case Thread::kRunning:
      DS2LOG(Debug, "not resuming tid %" PRI_PID ", already in state %s",
             thread->tid(), Stringify::ThreadState(thread->state()));
      break;

    case Thread::kStopped:
    case Thread::kStepped: {
      Architecture::CPUState state;
      thread->readCPUState(state);
      DS2LOG(Debug,
             "resuming tid %" PRI_PID " in state %s from pc %" PRI_PTR
             " with signal %d",
             thread->tid(), Stringify::ThreadState(thread->state()),
             PRI_PTR_CAST(state.pc()), signal);
      ErrorCode error = thread->resume(signal);
      if (error != kSuccess) {
        DS2LOG(Warning, "failed resuming tid %" PRI_PID ", error=%s",
               thread->tid(), Stringify::Error(error));
      }
    } break;
    }
  });

  return kSuccess;
}

// ELF only
ErrorCode ProcessBase::getAuxiliaryVector(std::string &auxv) {
  return kErrorUnsupported;
}

uint64_t ProcessBase::getAuxiliaryVectorValue(uint64_t type) { return 0; }

ErrorCode
ProcessBase::enumerateThreads(std::function<void(Thread *)> const &cb) const {
  if (_pid == kAnyProcessId)
    return kErrorProcessNotFound;

  for (auto const &it : _threads) {
    static_cast<ThreadBase *>(it.second)->updateState();
    cb(it.second);
  }

  return kSuccess;
}

void ProcessBase::getThreadIds(std::vector<ThreadId> &tids) {
  tids.clear();
  for (auto const &it : _threads) {
    tids.push_back(it.first);
  }
}

ds2::Target::Thread *ProcessBase::thread(ThreadId tid) const {
  auto it = _threads.find(tid);
  return (it == _threads.end()) ? nullptr : it->second;
}

ErrorCode ProcessBase::enumerateMappedFiles(
    std::function<void(MappedFileInfo const &)> const &cb) {
  // TODO: This is only looking for libraries loaded in memory, not all mapped
  // files.
  return enumerateSharedLibraries([&](SharedLibraryInfo const &library) {
#if defined(OS_POSIX)
    uint64_t base = library.svr4.baseAddress;
#elif defined(OS_WIN32)
    uint64_t base = library.sections[0];
#endif

    cb({library.path, base, 0});
  });
}

ErrorCode ProcessBase::readMemoryBuffer(Address const &address, size_t length,
                                        ByteVector &buffer) {
  if (_pid == kAnyProcessId)
    return kErrorProcessNotFound;
  else if (!address.valid())
    return kErrorInvalidArgument;

  buffer.resize(length);

  size_t nread;
  ErrorCode error = readMemory(address, buffer.data(), length, &nread);
  if (error != kSuccess) {
    buffer.clear();
    return error;
  }

  buffer.resize(nread);
  return kSuccess;
}

ErrorCode ProcessBase::writeMemoryBuffer(Address const &address,
                                         ByteVector const &buffer,
                                         size_t *nwritten) {
  if (_pid == kAnyProcessId)
    return kErrorProcessNotFound;
  else if (!address.valid())
    return kErrorInvalidArgument;

  return writeMemory(address, buffer.data(), buffer.size(), nwritten);
}

ErrorCode ProcessBase::writeMemoryBuffer(Address const &address,
                                         ByteVector const &buffer,
                                         size_t length, size_t *nwritten) {
  if (_pid == kAnyProcessId)
    return kErrorProcessNotFound;
  else if (!address.valid())
    return kErrorInvalidArgument;

  if (length > buffer.size()) {
    length = buffer.size();
  }

  return writeMemory(address, buffer.data(), length, nwritten);
}

void ProcessBase::insert(ThreadBase *thread) {
  if (!_threads
           .insert(std::make_pair(thread->tid(), static_cast<Thread *>(thread)))
           .second)
    return;

  DS2LOG(Debug, "[new Thread %" PRI_PTR " (LWP %" PRIu64 ")]",
         PRI_PTR_CAST(thread), (uint64_t)thread->tid());
}

void ProcessBase::removeThread(ThreadId tid) {
  auto it = _threads.find(tid);
  if (it == _threads.end())
    return;

  Thread *thread = it->second;
  _threads.erase(it);

  DS2LOG(Debug, "[delete Thread %" PRI_PTR " (LWP %" PRIu64 ") exited]",
         PRI_PTR_CAST(thread), (uint64_t)thread->tid());

  delete thread;
}

void ProcessBase::remove(ThreadBase *thread) { removeThread(thread->tid()); }

ErrorCode ProcessBase::beforeResume() {
  if (!isAlive())
    return kErrorProcessNotFound;

  //
  // Enable software breakpoints.
  //
  BreakpointManager *bpm = softwareBreakpointManager();
  if (bpm != nullptr) {
    bpm->enable();
  }

  enumerateThreads([&](Thread *thread) { thread->beforeResume(); });

  return kSuccess;
}

ErrorCode ProcessBase::afterResume() {
  if (!isAlive()) {
    return kSuccess;
  }

  // Disable breakpoints and try to hit software breakpoints.
  BreakpointManager *swBpm = softwareBreakpointManager();
  if (swBpm != nullptr) {
    for (auto it : _threads) {
      BreakpointManager::Site site;
      if (swBpm->hit(it.second, site) >= 0) {
        DS2LOG(Debug, "hit breakpoint for tid %" PRI_PID, it.second->tid());
      }
    }
    swBpm->disable();
  }

  BreakpointManager *hwBpm = hardwareBreakpointManager();
  if (hwBpm != nullptr) {
    hwBpm->disable();
  }

  return kSuccess;
}

SoftwareBreakpointManager *ProcessBase::softwareBreakpointManager() const {
  if (!_softwareBreakpointManager) {
    _softwareBreakpointManager = ds2::make_unique<SoftwareBreakpointManager>(
        const_cast<ProcessBase *>(this));
  }

  return _softwareBreakpointManager.get();
}

HardwareBreakpointManager *ProcessBase::hardwareBreakpointManager() const {
  if (!_hardwareBreakpointManager) {
    _hardwareBreakpointManager = ds2::make_unique<HardwareBreakpointManager>(
        const_cast<ProcessBase *>(this));
  }

  return _hardwareBreakpointManager.get();
}

void ProcessBase::prepareForDetach() {
  SoftwareBreakpointManager *bpm = softwareBreakpointManager();
  if (bpm != nullptr) {
    bpm->clear();
  }
}
} // namespace Target
} // namespace ds2
