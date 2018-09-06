//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Core/BreakpointManager.h"
#include "DebugServer2/Host/Linux/ExtraWrappers.h"
#include "DebugServer2/Host/Linux/PTrace.h"
#include "DebugServer2/Host/Linux/ProcFS.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Utils/Log.h"
#include "DebugServer2/Utils/String.h"
#include "DebugServer2/Utils/Stringify.h"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <elf.h>
#include <limits>
#include <sys/ptrace.h>
#include <sys/wait.h>
#if defined(HAVE_PROCESS_VM_READV) || defined(HAVE_PROCESS_VM_WRITEV)
#include <sys/uio.h>
#endif
#include <unistd.h>

using ds2::Host::Platform;
using ds2::Host::Linux::ProcFS;
using ds2::Host::Linux::PTrace;
using ds2::Utils::Stringify;

#define super ds2::Target::POSIX::ELFProcess

namespace ds2 {
namespace Target {
namespace Linux {

ErrorCode Process::attach(int waitStatus) {
  if (waitStatus <= 0) {
    CHK(ptrace().attach(_pid));
    _flags |= kFlagAttachedProcess;
    CHK(ptrace().wait(_pid, &waitStatus));
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

ErrorCode Process::readMemory(Address const &address, void *data, size_t length,
                              size_t *count) {
#if defined(HAVE_PROCESS_VM_READV)
  // Using process_vm_readv() is faster than using ptrace() because we can do
  // bigger reads that ptrace() (which can only read a word at a time); the
  // drawback is that process_vm_readv() cannot bypass page-level permissions
  // like ptrace() can.
  // This is why for reads smaller than word-size we go straight to the
  // fallback so we reduce the number of possible process_vm_readv() failures.
  // The most common occurence of this is when writing breakpoints.

  if (length > sizeof(uintptr_t)) {
    struct iovec local_iov = {data, length};
    struct iovec remote_iov = {reinterpret_cast<void *>(address.value()),
                               length};
    auto id = _currentThread == nullptr ? _pid : _currentThread->tid();

    ssize_t ret = process_vm_readv(id, &local_iov, 1, &remote_iov, 1, 0);
    if (ret >= 0) {
      if (count != nullptr) {
        *count = ret;
      }
      return kSuccess;
    }
  }
#endif

  // Fallback to super::readMemory, which uses ptrace(2).
  return super::readMemory(address, data, length, count);
}

ErrorCode Process::writeMemory(Address const &address, void const *data,
                               size_t length, size_t *count) {
#if defined(HAVE_PROCESS_VM_WRITEV)
  // See comment in Process::readMemory.
  if (length > sizeof(uintptr_t)) {
    struct iovec local_iov = {const_cast<void *>(data), length};
    struct iovec remote_iov = {reinterpret_cast<void *>(address.value()),
                               length};
    auto id = _currentThread == nullptr ? _pid : _currentThread->tid();

    ssize_t ret = process_vm_writev(id, &local_iov, 1, &remote_iov, 1, 0);
    if (ret >= 0) {
      if (count != nullptr) {
        *count = ret;
      }
      return kSuccess;
    }
  }
#endif

  // Fallback to super::writeMemory, which uses ptrace(2).
  return super::writeMemory(address, data, length, count);
}

ErrorCode Process::checkMemoryErrorCode(uint64_t address) {
  // mmap() returns MAP_FAILED thanks to the libc wrapper. Here we don't have
  // any, so we get the raw kernel return value, which is the address of the
  // newly allocated pages, if the call succeeds, or -errno if the call fails.
  if (address & (Platform::GetPageSize() - 1)) {
    int error = -address;
    DS2LOG(Debug, "mmap failed with errno=%s", Stringify::Errno(error));
    return Platform::TranslateError(error);
  }

  return kSuccess;
}

ErrorCode Process::wait() {
  int status, signal;
  bool stepping;
  ProcessInfo info;
  ThreadId tid;

  // We have at least one thread when we start waiting on a process.
  DS2ASSERT(!_threads.empty());

  while (!_threads.empty()) {
    tid = blocking_waitpid(-1, &status, __WALL);
    if (tid <= 0) {
      return kErrorProcessNotFound;
    }

    DS2LOG(Debug, "tid %" PRI_PID " %s", tid, Stringify::WaitStatus(status));

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

    stepping = _currentThread->_state == Thread::kStepped;
    _currentThread->updateStopInfo(status);

    switch (_currentThread->_stopInfo.event) {
    case StopInfo::kEventNone:
      // If _stopInfo.event is kEventNone it means the thread is either
      // 1. stopped because another thread of the inferior stopped (e.g. because
      //    of a breakpoint or received a signal, etc) and ds2 sent a SIGSTOP
      //    to pause this thread while the other thread's stop is evaluated. In
      //    this case we just resume the thread.
      // 2. stopped because this thread called clone(2) (or similar e.g.
      //    pthread_create). In this case we have two subcases:
      //    a. The thread was running and hit the clone/spawn and thus was
      //       stopped with a SIGTRAP. We just resume it and goto
      //       continue_waiting. The loop will continue and the next waitpid
      //       will handle the freshly spawned thread. In this case we call
      //       resume as well.
      //    b. We were single stepping (c.f. the `stepping` var) and
      //       were stopped by a SIGTRAP for the step while simultaneously
      //       being stopped via the SIGTRAP for the clone/spawn.
      //       In this case we can run into a race condition with the
      //       waitpid(3) call and thus have to handle the cloned thread
      //       and the currentThread at once. So we call step on the
      //       _currentThread and then beforeResume the cloned thread.
      //
      // We also have another theoretically possible case where we are not
      // stopped by a clone but still stepping while encountering a kEventNone.
      // This should not happen and we just assert that it does not.

      if (_currentThread->_stopInfo.reason == StopInfo::kReasonThreadSpawn &&
          stepping) { //(2b)
        unsigned long spawnedThreadIdData;
        CHK(ptrace().getEventMessage(_pid, spawnedThreadIdData));
        auto const spawnedThreadId = static_cast<ThreadId>(spawnedThreadIdData);

        if (_threads.find(spawnedThreadId) == _threads.end()) {
          // If the newly cloned thread does not yet exist within the _threads
          // vector then we have not yet seen it with waitpid(2). The most
          // recent
          // waitpid(2) returns a status guaranteeing a clone event did happen.
          // Therefore, we wait and block until we see the cloned thread.
          int spawnedThreadStatus;
          ThreadId returnedThreadId =
              blocking_waitpid(spawnedThreadId, &spawnedThreadStatus, __WALL);
          if (spawnedThreadId != returnedThreadId)
            return kErrorProcessNotFound;
          DS2LOG(Debug, "child thread tid %d %s", returnedThreadId,
                 Stringify::WaitStatus(spawnedThreadStatus));

          _currentThread->step();
          auto newThread = new Thread(this, returnedThreadId);
          newThread->beforeResume();
        } else {
          // The new thread corresponding to this kReasonThreadSpawn was already
          // caught by waitpid(2). Therefore, we only need to step the current
          // thread.
          _currentThread->step();
        }
      } else if (stepping) {
        // We should never see a case where we're:
        //   1. stopped for event kEventNone
        //   2. stepping
        //   3. not stopped for reason kReasonThreadSpawn
        DS2BUG("inconsistent thread stop info");
      } else {
        _currentThread->resume(); // (1) and (2a)
      }
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

      DS2LOG(Debug, "stopped tid=%" PRI_PID " status=%#x signal=%s", tid,
             status, Stringify::Signal(signal));

      if (_passthruSignals.find(signal) != _passthruSignals.end()) {
        DS2LOG(Debug, "%s passed through to thread %" PRI_PID ", not stopping",
               Stringify::Signal(signal), tid);
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

ds2::Host::Linux::PTrace &Process::ptrace() const {
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
  ProcFS::ReadProcessInfo(_pid, _info);

  //
  // Call super::updateInfo, in turn it will call updateAuxiliaryVector.
  //
  ErrorCode error = super::updateInfo();
  if (error != kSuccess && error != kErrorAlreadyExist) {
    return error;
  }

  return kSuccess;
}

ErrorCode Process::getMemoryRegionInfo(Address const &address,
                                       MemoryRegionInfo &info) {
  if (!address.valid()) {
    return kErrorInvalidArgument;
  }

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
    char name[PATH_MAX + 1];
    int nread;

    if (::fgets(buf, sizeof(buf), fp) == nullptr) {
      if (::feof(fp)) {
        break;
      } else {
        DS2ASSERT(errno != 0);
        return Platform::TranslateError();
      }
    }

    if (::sscanf(buf,
                 "%" PRIx64 "-%" PRIx64 " %c%c%c%c %" PRIx64 " %x:%x %" PRIu64
                 " %" STR(PATH_MAX) "s%n",
                 &start, &end, &r, &w, &x, &p, &offset, &devMinor, &devMajor,
                 &inode, name, &nread) != 11) {
      continue;
    }

    if (address >= last && address < start) {
      //
      // A hole.
      //
      info.start = last;
      info.length = start - last;
      info.name = name;
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
      info.name = name;
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
    if (error != kSuccess && error != kErrorAlreadyExist) {
      return error;
    }

    if (CPUTypeIs64Bit(_info.cpuType)) {
      info.length = std::numeric_limits<uint64_t>::max() - info.start;
    } else {
      info.length = std::numeric_limits<uint32_t>::max() - info.start;
    }
  }

  return kSuccess;
}

ErrorCode Process::executeCode(ByteVector const &codestr, uint64_t &result) {
  ProcessInfo info;

  CHK(getInfo(info));
  CHK(ptrace().execute(_currentThread->tid(), info, &codestr[0], codestr.size(),
                       result));

  return kSuccess;
}
} // namespace Linux
} // namespace Target
} // namespace ds2
