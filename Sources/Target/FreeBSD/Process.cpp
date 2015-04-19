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

#include "DebugServer2/Target/FreeBSD/Process.h"
#include "DebugServer2/Target/FreeBSD/Thread.h"
#include "DebugServer2/Host/FreeBSD/PTrace.h"
#include "DebugServer2/Host/FreeBSD/ProcStat.h"
#include "DebugServer2/Host/POSIX/AsyncProcessWaiter.h"
#include "DebugServer2/BreakpointManager.h"
#include "DebugServer2/Utils/Log.h"

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <elf.h>
#include <limits>
#include <sys/ptrace.h>
#include <sys/wait.h>

using ds2::Host::FreeBSD::PTrace;
using ds2::Host::FreeBSD::ProcStat;

#define super ds2::Target::POSIX::ELFProcess

namespace ds2 {
namespace Target {
namespace FreeBSD {

Process::Process()
    : super(), _breakpointManager(nullptr), _watchpointManager(nullptr),
      _terminated(false) {}

Process::~Process() { terminate(); }

ErrorCode Process::initialize(ProcessId pid, uint32_t flags) {
  //
  // Wait the main thread.
  //
  int status;
  ErrorCode error = ptrace().wait(pid, true, &status);
  DS2LOG(Target, Error, "wait result=%d", error);
  if (error != kSuccess)
    return error;

  ptrace().traceThat(pid);

  error = super::initialize(pid, flags);
  DS2LOG(Target, Error, "initialize=%d", error);
  if (error != kSuccess)
    return error;

  return attach(status);
}

ErrorCode Process::attach(bool reattach) { return attach(reattach ? 0 : -1); }

ErrorCode Process::attach(int waitStatus) {
  struct ptrace_lwpinfo lwpinfo;

  if (waitStatus <= 0) {
    ErrorCode error = ptrace().attach(_pid);
    DS2LOG(Target, Debug, "ptrace attach error=%d", error);
    if (error != kSuccess)
      return error;

    _flags |= kFlagAttachedProcess;

    error = ptrace().wait(_pid, true, &waitStatus);
    fprintf(stderr, "ptrace wait error=%d\n", error);
    if (error != kSuccess)
      return error;
    ptrace().traceThat(_pid);
  }

  if (_flags & kFlagAttachedProcess) {
    fprintf(stderr, "kFlagAttachedProcess = YESS\n");
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

      ProcStat::EnumerateThreads(_pid, [&](pid_t tid) {
        //
        // Attach the thread to the debugger and wait
        //
        fprintf(stderr, "new thread: tid=%d\n", tid);
        if (thread(tid) != nullptr || tid == _pid)
          return;

        keep_going = true;
        Thread *thread = new Thread(this, tid);
        if (ptrace().attach(tid) == kSuccess) {
          int status;

          ptrace().wait(tid, true, &status);
          _ptrace.getLwpInfo(tid, &lwpinfo);
          ptrace().traceThat(tid);
          thread->updateTrapInfo(status);
        }
      });
    }
  }

  //
  // Create the main thread, ourselves.
  //
  _ptrace.getLwpInfo(_pid, &lwpinfo);
  _currentThread = new Thread(this, lwpinfo.pl_lwpid);
  _currentThread->updateTrapInfo(waitStatus);


  return kSuccess;
}

ErrorCode Process::wait(int *rstatus, bool hang) {
  int status, signal;
  struct ptrace_lwpinfo lwpinfo;
  struct rusage rusage;
  ProcessInfo info;
  ErrorCode err;
  pid_t tid;

  // We have at least one thread when we start waiting on a process.
  DS2ASSERT(!_threads.empty());

continue_waiting:
  fprintf(stderr, "Process::wait(rstatus=%p, hang=%d)\n", rstatus, hang);
  err = super::wait(&status, hang);
  if (err != kSuccess)
    return err;

  fprintf(stderr, "stopped, status=%d\n", status);

  if (WIFEXITED(status)) {
    err = super::wait(&status, true);
    fprintf(stderr, "exit status=%d\n", status);
    _currentThread->updateTrapInfo(status);
    _terminated = true;
    if (rstatus != nullptr) {
      *rstatus = status;
    }

    return kSuccess;
  }


  _ptrace.getLwpInfo(_pid, &lwpinfo);
  fprintf(stderr, "lwpinfo id=%d\n", lwpinfo.pl_lwpid);
  fprintf(stderr, "lwpinfo siginfo si_code=%d\n", lwpinfo.pl_siginfo.si_code);
  fprintf(stderr, "lwpinfo flags=0x%08x\n", lwpinfo.pl_flags);

  tid = lwpinfo.pl_lwpid;
  auto threadIt = _threads.find(tid);

  DS2ASSERT(threadIt != _threads.end());
  _currentThread = threadIt->second;
  _currentThread->updateTrapInfo(status);

  switch (_currentThread->_trap.event) {
  case TrapInfo::kEventNone:
    switch (_currentThread->_trap.reason) {
    case TrapInfo::kReasonNone:
      ptrace().resume(ProcessThreadId(_pid, tid), info);
      goto continue_waiting;
    case TrapInfo::kReasonThreadExit:
      // Remove thread
      _threads.erase(tid);

      ptrace().resume(ProcessThreadId(_pid, tid), info);
      goto continue_waiting;

    case TrapInfo::kReasonThreadNew:
      // Rescan threads
      ProcStat::EnumerateThreads(_pid, [&](pid_t tid) {
          //
          // Attach the thread to the debugger and wait
          //
          auto threadIt = _threads.find(tid);
          if (threadIt != _threads.end())
            return;

          Thread *thread = new Thread(this, tid);
          if (ptrace().attach(tid) == kSuccess) {
            int status;

            ptrace().wait(tid, true, &status);
            _ptrace.getLwpInfo(tid, &lwpinfo);
            ptrace().traceThat(tid);
            thread->updateTrapInfo(status);
          }
      });


      ptrace().resume(ProcessThreadId(_pid, tid), info);
      goto continue_waiting;
    }

  case TrapInfo::kEventExit:
  case TrapInfo::kEventKill:
  case TrapInfo::kEventCoreDump:
    DS2LOG(Target, Debug, "thread %d is exiting", tid);

    //
    // Killing the main thread?
    //
    // Note(sas): This might be buggy; the main thread exiting
    // doesn't mean that the process is dying.
    //
    if (tid == _pid && _threads.size() == 1) {
      DS2LOG(Target, Debug, "last thread is exiting");
      break;
    }

    //
    // Remove and release the thread associated with this pid.
    //
    removeThread(tid);
    goto continue_waiting;

  case TrapInfo::kEventTrap:
    DS2LOG(Target, Debug, "stopped tid=%d status=%#x signal=%s", tid, status,
           strsignal(WSTOPSIG(status)));
    break;

  case TrapInfo::kEventStop:
    if (getInfo(info) != kSuccess) {
      DS2LOG(Target, Error, "couldn't get process info for pid %d", _pid);
      goto continue_waiting;
    }

    signal = _currentThread->_trap.signal;

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
        DS2LOG(Target, Debug,
               "%s due to special signal, tid=%d status=%#x signal=%s",
               stepping ? "stepping" : "resuming", tid, status,
               strsignal(signal));
      }

      ErrorCode error;
      if (stepping) {
        error = ptrace().step(ProcessThreadId(_pid, tid), info, signal);
      } else {
        error = ptrace().resume(ProcessThreadId(_pid, tid), info, signal);
      }

      if (error != kSuccess) {
        DS2LOG(Target, Warning, "cannot resume thread %d error=%d", tid,
               error);
      }

      goto continue_waiting;
    } else if (_passthruSignals.find(signal) != _passthruSignals.end()) {
      ptrace().resume(ProcessThreadId(_pid, tid), info, signal);
      goto continue_waiting;
    } else {
      //
      // This is a signal that we want to transmit back to the
      // debugger.
      //
      break;
    }
  }

  if (!(WIFEXITED(status) || WIFSIGNALED(status))) {
    //
    // Suspend the process, this must be done after updating
    // the thread trap info.
    //
    suspend();
  }

  if ((WIFEXITED(status) || WIFSIGNALED(status))) {
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

ErrorCode Process::suspend() {
  std::set<Thread *> threads;
  enumerateThreads([&](Thread *thread) { threads.insert(thread); });

  for (auto thread : threads) {
    Architecture::CPUState state;
    if (thread->state() != Thread::kRunning) {
      thread->readCPUState(state);
    }
    DS2LOG(Target, Debug, "tid %d state %d at pc %#llx", thread->tid(),
           thread->state(),
           thread->state() == Thread::kStopped ? (unsigned long long)state.pc()
                                               : 0);
    if (thread->state() == Thread::kRunning) {
      ErrorCode error;

      DS2LOG(Target, Debug, "suspending tid %d", thread->tid());
      error = thread->suspend();

      if (error == kSuccess) {
        DS2LOG(Target, Debug, "suspended tid %d at pc %#llx", thread->tid(),
               (unsigned long long)state.pc());
        thread->readCPUState(state);
      } else if (error == kErrorProcessNotFound) {
        //
        // Thread is dead.
        //
        removeThread(thread->tid());
        DS2LOG(Target, Debug, "tried to suspended tid %d which is already dead",
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

ErrorCode Process::updateAuxiliaryVector() {
  return kSuccess;
}

ErrorCode Process::enumerateAuxiliaryVector(std::function<void(Support::ELFSupport::AuxiliaryVectorEntry const &)> const &cb) {
  if (!ProcStat::EnumerateAuxiliaryVector(_pid, cb))
    return kSuccess;

  return kErrorUnknown;
}

ErrorCode Process::updateInfo() {
  //
  // Some info like parent pid, OS vendor, etc is obtained via libprocstat.
  //
  ProcStat::GetProcessInfo(_info.pid, _info);

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

  std::vector<MemoryRegionInfo> map;

  info.clear();
  ProcStat::GetProcessMap(_pid, map);

  for (MemoryRegionInfo i : map) {
    if (address >= i.start && address <= (i.start + i.length)) {
      info = i;
      return kSuccess;
    }
  }

  return kErrorNotFound;
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
