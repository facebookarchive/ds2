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
#include "DebugServer2/Host/Linux/ExtraWrappers.h"
#include "DebugServer2/Host/Linux/PTrace.h"
#include "DebugServer2/Host/Linux/ProcFS.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Support/Stringify.h"
#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Utils/Log.h"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
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

ErrorCode Process::initialize(ProcessId pid, uint32_t flags) {
  //
  // Wait the main thread.
  //
  int status;
  ErrorCode error = ptrace().wait(pid, &status);
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
          ptrace().wait(tid, &status);
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
  do {
    ret = ::waitpid(pid, status, flags);
  } while (ret == -1 && errno == EINTR);

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

ErrorCode Process::wait() {
  int status, signal;
  ProcessInfo info;
  ThreadId tid;

  // We have at least one thread when we start waiting on a process.
  DS2ASSERT(!_threads.empty());

  while (!_threads.empty()) {
    tid = blocking_waitpid(-1, &status, __WALL);
    DS2LOG(Debug, "wait tid=%d status=%#x", tid, status);

    if (tid <= 0)
      return kErrorProcessNotFound;

    auto threadIt = _threads.find(tid);

    if (threadIt == _threads.end()) {
      // If we don't know about this thread yet, but it has a WIFEXITED() or a
      // WIFSIGNALED() status (i.e.: it terminated), it means we already
      // cleaned up the thread object (e.g.: in Process::suspend), but we
      // hadn't waitpid()'d it yet. Avoid re-creating a Thread object here.
      if (WIFEXITED(status) || WIFSIGNALED(status)) {
        goto continue_waiting;
      }

      // A new thread has appeared that we didn't know about. Create the
      // Thread object and return.
      DS2LOG(Debug, "creating new thread tid=%d", tid);
      _currentThread = new Thread(this, tid);
      return kSuccess;
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

      if (_passthruSignals.find(signal) != _passthruSignals.end()) {
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
    uint64_t start, end;
    char r, w, x, p;
    uint64_t offset;
    unsigned int devMinor, devMajor;
    uint64_t inode;
    int nread;

    if (std::fgets(buf, sizeof(buf), fp) == nullptr)
      break;

    if (std::sscanf(buf, "%" PRIx64 "-%" PRIx64 " %c%c%c%c %" PRIx64
                         " %x:%x %" PRIu64 " %n",
                    &start, &end, &r, &w, &x, &p, &offset, &devMinor, &devMajor,
                    &inode, &nread) != 10) {
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
}
}
}
