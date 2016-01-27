//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#define __DS2_LOG_CLASS_NAME__ "Target::Process"

#include "DebugServer2/BreakpointManager.h"
#include "DebugServer2/Host/Linux/ExtraWrappers.h"
#include "DebugServer2/Host/Linux/PTrace.h"
#include "DebugServer2/Host/Linux/ProcFS.h"
#include "DebugServer2/Host/POSIX/AsyncProcessWaiter.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Support/Stringify.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Utils/Log.h"

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <elf.h>
#include <limits>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>

using ds2::Host::Linux::PTrace;
using ds2::Host::Linux::ProcFS;
using ds2::Host::Platform;
using ds2::Support::Stringify;

#define super ds2::Target::POSIX::ELFProcess

namespace ds2 {
namespace Target {
namespace Linux {

Process::Process()
    : super(), _breakpointManager(nullptr), _watchpointManager(nullptr),
      _terminated(false) {}

Process::~Process() = default;

ErrorCode Process::initialize(ProcessId pid, uint32_t flags) {
  //
  // Wait the main thread.
  //
  int status;
  ErrorCode error = ptrace().wait(pid, true, &status);
  if (error != kSuccess)
    return error;

  ptrace().traceThat(pid);

  error = super::initialize(pid, flags);
  if (error != kSuccess)
    return error;

  return attach(status);
}

ErrorCode Process::attach(int waitStatus) {
  if (waitStatus <= 0) {
    ErrorCode error = ptrace().attach(_pid);
    if (error != kSuccess)
      return error;

    _flags |= kFlagAttachedProcess;

    error = ptrace().wait(_pid, true, &waitStatus);
    if (error != kSuccess)
      return error;
    ptrace().traceThat(_pid);
  }

  if (_flags & kFlagAttachedProcess) {
    //
    // Enumerate all the tasks and create a Thread
    // object for every entry.
    //
    bool keepGoing = true;

    //
    // Try to find threads of the newly attached process in multiple
    // rounds so that we avoid race conditions with threads being
    // created before we stop the creator.
    //
    while (keepGoing) {
      keepGoing = false;

      ProcFS::EnumerateThreads(_pid, [&](pid_t tid) {
        //
        // Attach the thread to the debugger and wait
        //
        if (thread(tid) != nullptr || tid == _pid)
          return;

        keepGoing = true;
        auto thread = new Thread(this, tid);
        if (ptrace().attach(tid) == kSuccess) {
          int status;
          ptrace().wait(tid, true, &status);
          ptrace().traceThat(tid);
          thread->updateStopInfo(status);
        }
      });
    }
  }

  //
  // Create the main thread, ourselves.
  //
  _currentThread = new Thread(this, _pid);
  _currentThread->updateStopInfo(waitStatus);

  return kSuccess;
}

static pid_t blocking_waitpid(pid_t pid, int *status, int flags) {
  pid_t ret;
  sigset_t block_mask, org_mask, wake_mask;

  sigemptyset(&wake_mask);
  sigfillset(&block_mask);

  // Block all signals while waiting.
  sigprocmask(SIG_BLOCK, &block_mask, &org_mask);

  for (;;) {
    ret = ::waitpid(pid, status, flags);
    if (ret > 0 || (ret == -1 && errno != ECHILD))
      break;

    if ((flags & (__WCLONE | WNOHANG)) == __WCLONE) {
      sigsuspend(&wake_mask);
    }
  }

  sigprocmask(SIG_SETMASK, &org_mask, nullptr);

  return ret;
}

ErrorCode Process::checkMemoryErrorCode(uint64_t address) {
  // mmap() returns MAP_FAILED thanks to the libc wrapper. Here we don't have
  // any, so we get the raw kernel return value, which is the address of the
  // newly allocated pages, if the call succeeds, or -errno if the call fails.

  int pgsz = getpagesize();

  if (address & (pgsz - 1)) {
    int error = -address;
    DS2LOG(Debug, "mmap failed with errno=%s", Stringify::Errno(error));
    return Platform::TranslateError(error);
  }

  return kSuccess;
}

ErrorCode Process::wait(int *rstatus, bool hang) {
  int status, signal;
  ProcessInfo info;
  ThreadId tid;

  // We have at least one thread when we start waiting on a process.
  DS2ASSERT(!_threads.empty());

  while (!_threads.empty()) {
    tid = blocking_waitpid(-1, &status, __WALL | (hang ? 0 : WNOHANG));
    DS2LOG(Debug, "wait tid=%d status=%#x", tid, status);

    if (tid <= 0)
      return kErrorProcessNotFound;

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
        break;
      }

      //
      // Remove and release the thread associated with this pid.
      //
      removeThread(tid);
      goto continue_waiting;

    case StopInfo::kEventStop:
      signal = _currentThread->_stopInfo.signal;

      DS2LOG(Debug, "stopped tid=%d status=%#x signal=%s", tid, status,
             Stringify::Signal(signal));

      if (signal == SIGSTOP || signal == SIGCHLD || signal == SIGRTMIN) {
        //
        // Silently ignore SIGSTOP, SIGCHLD and SIGRTMIN (this
        // last one used for thread cancellation) and continue.
        //
        // Note(oba): The SIGRTMIN defines expands to a glibc
        // call, this due to the fact the POSIX standard does
        // not mandate that SIGRT* defines to be user-land
        // constants.
        //
        // Note(sas): This is probably partially dead code as
        // ptrace().step() doesn't work on ARM.
        //
        // Note(sas): Single-step detection should be higher up, not
        // only for SIGSTOP, SIGCHLD and SIGRTMIN, but for every
        // signal that we choose to ignore.
        //
        bool stepping = (_currentThread->state() == Thread::kStepped);

        if (signal == SIGSTOP) {
          signal = 0;
        } else {
          DS2LOG(Debug, "%s due to special signal, tid=%d status=%#x signal=%s",
                 stepping ? "stepping" : "resuming", tid, status,
                 Stringify::Signal(signal));
        }

        ErrorCode error;
        if (stepping) {
          error = _currentThread->step(signal);
        } else {
          error = _currentThread->resume(signal);
        }

        if (error != kSuccess) {
          DS2LOG(Warning, "cannot resume thread %d, error=%d", tid, error);
        }

        goto continue_waiting;
      } else if (_passthruSignals.find(signal) != _passthruSignals.end()) {
        _currentThread->resume(signal);
        goto continue_waiting;
      } else {
        //
        // This is a signal that we want to transmit back to the
        // debugger.
        //
        break;
      }
    }

    break;

  continue_waiting:
    _currentThread = nullptr;
    continue;
  }

  if (!(WIFEXITED(status) || WIFSIGNALED(status)) || tid != _pid) {
    //
    // Suspend the process, this must be done after updating
    // the thread trap info.
    //
    suspend();
  }

  if ((WIFEXITED(status) || WIFSIGNALED(status)) && tid == _pid) {
    _terminated = true;
  }

  if (rstatus != nullptr) {
    *rstatus = status;
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

bool Process::isAlive() const {
  if (_terminated)
    return false;

  auto it = _threads.find(_pid);
  if (it == _threads.end())
    return false;

  switch (it->second->state()) {
  case Thread::kInvalid:
  case Thread::kTerminated:
    return false;
  default:
    break;
  }

  return super::isAlive();
}

ErrorCode Process::suspend() {
  std::set<Thread *> threads;
  enumerateThreads([&](Thread *thread) { threads.insert(thread); });

  for (auto thread : threads) {
    Architecture::CPUState state;
    if (thread->state() != Thread::kRunning) {
      thread->readCPUState(state);
    }
    DS2LOG(Debug, "tid %d state %d at pc %#llx", thread->tid(), thread->state(),
           thread->state() == Thread::kStopped ? (unsigned long long)state.pc()
                                               : 0);
    if (thread->state() == Thread::kRunning) {
      ErrorCode error;

      DS2LOG(Debug, "suspending tid %d", thread->tid());
      error = thread->suspend();

      if (error == kSuccess) {
        DS2LOG(Debug, "suspended tid %d at pc %#llx", thread->tid(),
               (unsigned long long)state.pc());
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

ErrorCode Process::resume(int signal, std::set<Thread *> const &excluded) {
  enumerateThreads([&](Thread *thread) {
    if (excluded.find(thread) != excluded.end())
      return;

    if (thread->state() == Thread::kStopped ||
        thread->state() == Thread::kStepped) {
      Architecture::CPUState state;
      thread->readCPUState(state);
      DS2LOG(Debug, "resuming tid %d from pc %#llx", thread->tid(),
             (unsigned long long)state.pc());
      ErrorCode error = thread->resume(signal);
      if (error != kSuccess) {
        DS2LOG(Warning, "failed resuming tid %d, error=%d", thread->tid(),
               error);
      }
    }
  });

  return kSuccess;
}

ds2::Host::POSIX::PTrace &Process::ptrace() const {
  return const_cast<Process *>(this)->_ptrace;
}

ErrorCode Process::updateAuxiliaryVector() {
  ErrorCode error = super::updateAuxiliaryVector();
  if (error != kSuccess) {
    return (error == kErrorAlreadyExist) ? kSuccess : error;
  }

  FILE *fp = ProcFS::OpenFILE(_pid, "auxv");
  if (fp == nullptr) {
    switch (errno) {
    case ESRCH:
      return kErrorProcessNotFound;
    default:
      return kErrorUnsupported;
    }
  }

  static size_t const chunkSize = 1024;
  size_t auxvSize = 0;
  for (;;) {
    size_t size = _auxiliaryVector.size();
    _auxiliaryVector.resize(size + chunkSize);

    ssize_t nread = std::fread(&_auxiliaryVector[size], 1, chunkSize, fp);
    if (nread <= 0)
      break;

    auxvSize += nread;
  }
  std::fclose(fp);

  _auxiliaryVector.resize(auxvSize);

  return kSuccess;
}

ErrorCode Process::updateInfo() {
  //
  // Some info like parent pid, OS vendor, etc is obtained via /proc.
  //
  ProcFS::ReadProcessInfo(_info.pid, _info);

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

  FILE *fp = ProcFS::OpenFILE(_pid, "maps");
  if (fp == nullptr) {
    return Platform::TranslateError();
  }

  uint64_t last = 0;
  bool found = false;

  while (!found) {
    // Each line can contain one path and some additional addresses and
    // such, so PATH_MAX * 2 should be enough.
    char buf[PATH_MAX * 2];
    unsigned long long start, end;
    char r, w, x, p;
    unsigned long long offset;
    unsigned int devMinor, devMajor;
    unsigned long long inode;
    int nread;

    if (std::fgets(buf, sizeof(buf), fp) == nullptr)
      break;

    if (std::sscanf(buf, "%llx-%llx %c%c%c%c %llx %x:%x %llu %n", &start, &end,
                    &r, &w, &x, &p, &offset, &devMinor, &devMajor, &inode,
                    &nread) != 10) {
      continue;
    }

    if (address >= last && address <= start) {
      //
      // A hole.
      //
      info.start = last;
      info.length = start - last;
      found = true;
    } else if (address >= start && address < end) {
      //
      // A defined region.
      //
      info.start = start;
      info.length = end - start;
      info.protection = 0;
      if (r == 'r')
        info.protection |= ds2::kProtectionRead;
      if (w == 'w')
        info.protection |= ds2::kProtectionWrite;
      if (x == 'x')
        info.protection |= ds2::kProtectionExecute;
      while (buf[nread] != '\0' && std::isspace(buf[nread]))
        ++nread;
      info.backingFile = buf + nread;
      info.backingFileOffset = offset;
      info.backingFileInode = inode;
      found = true;
    } else {
      last = end;
    }
  }
  std::fclose(fp);

  if (!found) {
    info.start = last;

    //
    // We need to obtain the end of the address space, first
    // we need to know if it's 64-bit.
    //
    ErrorCode error = updateInfo();
    if (error != kSuccess && error != kErrorAlreadyExist)
      return error;

    if (CPUTypeIs64Bit(_info.cpuType)) {
      info.length = std::numeric_limits<uint64_t>::max() - info.start;
    } else {
      info.length = std::numeric_limits<uint32_t>::max() - info.start;
    }
  }

  return kSuccess;
}

ErrorCode Process::readString(Address const &address, std::string &str,
                              size_t length, size_t *count) {
  if (_currentThread == nullptr)
    return super::readString(address, str, length, count);

  return ptrace().readString(_currentThread->tid(), address, str, length,
                             count);
}

ErrorCode Process::readMemory(Address const &address, void *data, size_t length,
                              size_t *count) {
  if (_currentThread == nullptr)
    return super::readMemory(address, data, length, count);

  return ptrace().readMemory(_currentThread->tid(), address, data, length,
                             count);
}

ErrorCode Process::writeMemory(Address const &address, void const *data,
                               size_t length, size_t *count) {
  if (_currentThread == nullptr)
    return super::writeMemory(address, data, length, count);

  return ptrace().writeMemory(_currentThread->tid(), address, data, length,
                              count);
}
}
}
}
