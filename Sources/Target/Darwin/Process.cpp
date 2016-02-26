//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#define __DS2_LOG_CLASS_NAME__ "Target::Process"

#include "DebugServer2/Target/Process.h"
#include "DebugServer2/BreakpointManager.h"
#include "DebugServer2/Host/Darwin/LibProc.h"
#include "DebugServer2/Host/Darwin/Mach.h"
#include "DebugServer2/Host/Darwin/PTrace.h"
#include "DebugServer2/Target/Darwin/Thread.h"
#include "DebugServer2/Utils/Log.h"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/wait.h>

using ds2::Host::Darwin::PTrace;
using ds2::Host::Darwin::LibProc;

#define super ds2::Target::Darwin::MachOProcess

namespace ds2 {
namespace Target {
namespace Darwin {

ErrorCode Process::initialize(ProcessId pid, uint32_t flags) {
  //
  // Wait the main thread.
  //
  ErrorCode error;
  Host::Darwin::MachExcStatus status;

  error = mach().setupExceptionChannel(pid);
  if (error != kSuccess) {
    DS2LOG(Error, "Unable to setup the exception port");
    return error;
  }

  error = mach().readException(status, false);
  if (error != kSuccess) {
    DS2LOG(Error, "Unable to read the exception");
    return error;
  }

  error = ptrace().traceThat(pid);
  if (error != kSuccess) {
    return error;
  }

  ProcessInfo pinfo; // Unused arg
  error = ptrace().resume(ProcessThreadId(pid, pid), pinfo);
  if (error != kSuccess) {
    return error;
  }

  error = ds2::Target::ProcessBase::initialize(pid, flags);
  if (error != kSuccess) {
    return error;
  }

  _currentThread = new Thread(this, pid);
  _currentThread->updateStopInfo(status);

  return kSuccess;
}

ErrorCode Process::attach(int waitStatus) {

  if (waitStatus <= 0) {
    ErrorCode error = ptrace().attach(_pid);
    if (error != kSuccess) {
      return error;
    }

    _flags |= kFlagAttachedProcess;

    error = ptrace().wait(_pid, &waitStatus);
    if (error != kSuccess)
      return error;
    ptrace().traceThat(_pid);
  }

  if (_flags & kFlagAttachedProcess) {
    //
    // Enumerate all the tasks and create a Thread
    // object for every entry.
    //
    bool keep_going = true;

    //
    // Try to find threads of the newly attached process in multiple
    // rounds so that we avoid race conditions with threads being
    // created before we stop the creator.
    //
    while (keep_going) {
      keep_going = false;

      DS2BUG("not implemented");
    }
  }

  //
  // Create the main thread, ourselves.
  //
  _currentThread = new Thread(this, _pid);
  _currentThread->updateStopInfo(waitStatus);

  return kSuccess;
}

ErrorCode Process::wait() {
  int signal;
  ProcessInfo info;
  pid_t tid;

  // We have at least one thread when we start waiting on a process.
  DS2ASSERT(!_threads.empty());

  while (!_threads.empty()) {
    Host::Darwin::MachExcStatus status;

    ErrorCode error = mach().readException(status, false);
    if (error != kSuccess) {
      DS2LOG(Error, "Failed to get the breakpoint");
      return error;
    }

    enumerateThreads([&](Thread *thread) {
      if (mach().exceptionIsFromThread(ProcessThreadId(pid(), thread->tid())))
        tid = thread->tid();
    });

    auto threadIt = _threads.find(tid);

    if (threadIt == _threads.end()) {
      //
      // A new thread has appeared that we didn't know about. Create the
      // Thread object (this call has side effects that save the Thread in
      // the Process therefore we don't need to retain the pointer),
      // resume the thread and just continue waiting.
      //
      // There's no need to call traceThat() on the newly created thread
      // here because the ptrace flags are inherited when new threads
      // are created.
      //
      DS2LOG(Debug, "creating new thread tid=%d", tid);
      auto thread = new Thread(this, tid);
      thread->resume();
      goto continue_waiting;
    } else {
      _currentThread = threadIt->second;
    }

    _currentThread->updateStopInfo(status);

    switch (_currentThread->_stopInfo.event) {
    case StopInfo::kEventNone:
      _currentThread->resume();
      goto continue_waiting;
    case StopInfo::kEventExit:
    case StopInfo::kEventKill:
      DS2LOG(Debug, "thread %d is exiting", tid);

      //
      // Killing the main thread?
      //
      // Note(sas): This might be buggy; the main thread exiting
      // doesn't mean that the process is dying.
      //
      if (tid == _pid && _threads.size() == 1) {
        DS2LOG(Debug, "last thread is exiting");
        _terminated = true;
        break;
      }

      //
      // Remove and release the thread associated with this pid.
      //
      removeThread(tid);
      goto continue_waiting;

    case StopInfo::kEventStop:
      signal = _currentThread->_stopInfo.signal;

      if (_passthruSignals.find(signal) != _passthruSignals.end()) {
        ptrace().resume(ProcessThreadId(_pid, tid), info, signal);
        goto continue_waiting;
      } else {
        //
        // This is a signal that we want to transmit back to the
        // debugger.
        //
        suspend();
        break;
      }
    }

    // This thread need to be reported
    break;

  continue_waiting:
    _currentThread = nullptr;
  }

  return kSuccess;
}

ErrorCode Process::terminate() {
  ErrorCode error = super::terminate();
  if (error == kSuccess || error == kErrorProcessNotFound) {
    _terminated = !super::isAlive();
  }
  return error;
}

ErrorCode Process::suspend() {
  std::set<Thread *> threads;
  enumerateThreads([&](Thread *thread) { threads.insert(thread); });

  for (auto thread : threads) {
    Architecture::CPUState state;
    if (thread->state() != Thread::kRunning) {
      thread->readCPUState(state);
    }
    DS2LOG(Debug, "tid %d state %d at pc %#" PRIx64, thread->tid(),
           thread->state(),
           thread->state() == Thread::kStopped ? (uint64_t)state.pc() : 0);
    if (thread->state() == Thread::kRunning) {
      ErrorCode error;

      DS2LOG(Debug, "suspending tid %d", thread->tid());
      error = thread->suspend();

      if (error == kSuccess) {
        DS2LOG(Debug, "suspended tid %d at pc %#" PRIx64, thread->tid(),
               (uint64_t)state.pc());
        thread->readCPUState(state);
      } else if (error == kErrorProcessNotFound) {
        //
        // Thread is dead.
        //
        removeThread(thread->tid());
        DS2LOG(Debug, "tried to suspended tid %d which is already dead",
               thread->tid());
      } else {
        return error;
      }
    } else if (thread->state() == Thread::kTerminated) {
      //
      // Thread is dead.
      //
      removeThread(thread->tid());
    }
  }

  return kSuccess;
}

ds2::Host::POSIX::PTrace &Process::ptrace() const {
  return const_cast<Process *>(this)->_ptrace;
}

ErrorCode Process::updateAuxiliaryVector() { return kSuccess; }

ErrorCode Process::updateInfo() {
  //
  // Some info like parent pid, OS vendor, etc is obtained via libprocstat.
  //
  LibProc::GetProcessInfo(_info.pid, _info);

  //
  // Call super::updateInfo, in turn it will call updateAuxiliaryVector.
  //
  ErrorCode error = super::updateInfo();
  if (error != kSuccess && error != kErrorAlreadyExist)
    return error;

  return kSuccess;
}

ErrorCode Process::getMemoryRegionInfo(Address const &address,
                                       MemoryRegionInfo &info) {
  if (!address.valid())
    return kErrorInvalidArgument;

  info.clear();

  return mach().getProcessMemoryRegion(_info.pid, address, info);
}

ErrorCode Process::readString(Address const &address, std::string &str,
                              size_t length, size_t *count) {
  if (_currentThread == nullptr)
    return kErrorProcessNotFound;

  char buf[length];
  ErrorCode err;

  err = mach().readMemory(_currentThread->tid(), address, buf, length, count);
  if (err != kSuccess)
    return err;

  if (strnlen(buf, length) == length)
    return kErrorNameTooLong;

  str = std::string(buf);
  return kSuccess;
}

ErrorCode Process::readMemory(Address const &address, void *data, size_t length,
                              size_t *count) {
  if (_currentThread == nullptr)
    return kErrorProcessNotFound;

  return mach().readMemory(_currentThread->tid(), address, data, length, count);
}

ErrorCode Process::writeMemory(Address const &address, void const *data,
                               size_t length, size_t *count) {
  if (_currentThread == nullptr)
    return kErrorProcessNotFound;

  return mach().writeMemory(_currentThread->tid(), address, data, length,
                            count);
}

ErrorCode Process::afterResume() {
  // We are calling the Thread's afterResume from here, but it might make sense
  // to have this known by ThreadBase and call it from ProcessBase::resume.
  ErrorCode error = _currentThread->afterResume();
  if (error != kSuccess) {
    return error;
  }

  return super::afterResume();
}
}
}
}
