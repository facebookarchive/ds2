//
// Copyright (c) 2014, Facebook, Inc.
// Copyright (c) 2015, Jakub Klama <jakub@ixsystems.com>
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#define __DS2_LOG_CLASS_NAME__ "PTrace"

#include "DebugServer2/Host/FreeBSD/PTrace.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Host/POSIX/AsyncProcessWaiter.h"
#include "DebugServer2/Utils/Log.h"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <limits>
#include <sys/ptrace.h>
#include <sys/thr.h>

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace FreeBSD {

PTrace::PTrace() : _privateData(nullptr) {}

PTrace::~PTrace() { doneCPUState(); }

ErrorCode PTrace::traceMe(bool disableASLR) {
  if (wrapPtrace(PT_TRACE_ME, 0, nullptr, nullptr) < 0)
    return Platform::TranslateError();

  return kSuccess;
}

ErrorCode PTrace::traceThat(ProcessId pid) {
  if (pid <= 0)
    return kErrorInvalidArgument;

  return kSuccess;
}

ErrorCode PTrace::attach(ProcessId pid) {
  if (pid <= kAnyProcessId)
    return kErrorProcessNotFound;

  if (wrapPtrace(PT_ATTACH, pid, nullptr, nullptr) < 0)
    return Platform::TranslateError();

  return kSuccess;
}

ErrorCode PTrace::detach(ProcessId pid) {
  if (pid <= kAnyProcessId)
    return kErrorProcessNotFound;

  DS2LOG(Debug, "detaching from pid %llu", (unsigned long long)pid);
  if (wrapPtrace(PT_DETACH, pid, nullptr, nullptr) < 0)
    return Platform::TranslateError();

  return kSuccess;
}

ErrorCode PTrace::kill(ProcessThreadId const &ptid, int signal) {
  if (!ptid.valid())
    return kErrorInvalidArgument;

  int rc;
  if (!(ptid.tid <= kAnyThreadId)) {
    if (!(ptid.pid <= kAnyProcessId)) {
      rc = ::thr_kill2(ptid.pid, ptid.tid, signal);
    } else {
      rc = ::thr_kill(ptid.tid, signal);
    }
  } else {
    rc = ::kill(ptid.pid, signal);
  }

  if (rc < 0)
    return Platform::TranslateError();

  return kSuccess;
}

ErrorCode PTrace::readMemory(ProcessThreadId const &ptid,
                             Address const &address, void *buffer,
                             size_t length, size_t *count) {
  pid_t pid;

  if (!ptid.valid() || !address.valid())
    return kErrorInvalidArgument;

  if (!(ptid.tid <= kAnyThreadId)) {
    pid = ptid.tid;
  } else {
    pid = ptid.pid;
  }

  uintptr_t base = address.value();
  struct ptrace_io_desc desc;

  if (length == 0 || buffer == nullptr) {
    if (count != nullptr)
      *count = 0;

    return kSuccess;
  }

  desc.piod_op = PIOD_READ_D;
  desc.piod_offs = (void *)base;
  desc.piod_addr = buffer;
  desc.piod_len = length;

  if (wrapPtrace(PT_IO, pid, &desc, nullptr) < 0)
    return Platform::TranslateError();

  if (count != nullptr)
    *count = desc.piod_len;
  return kSuccess;
}

ErrorCode PTrace::writeMemory(ProcessThreadId const &ptid,
                              Address const &address, void const *buffer,
                              size_t length, size_t *count) {
  pid_t pid;

  if (!ptid.valid() || !address.valid())
    return kErrorInvalidArgument;

  if (!(ptid.tid <= kAnyThreadId)) {
    pid = ptid.tid;
  } else {
    pid = ptid.pid;
  }

  uintptr_t base = address;
  struct ptrace_io_desc desc;

  if (length == 0 || buffer == nullptr) {
    if (count != nullptr)
      *count = 0;

    return kSuccess;
  }

  desc.piod_op = PIOD_WRITE_D;
  desc.piod_offs = (void *)base;
  desc.piod_addr = (void *)buffer;
  desc.piod_len = length;

  if (wrapPtrace(PT_IO, pid, &desc, nullptr) < 0)
    return Platform::TranslateError();

  if (count != nullptr)
    *count = desc.piod_len;
  return kSuccess;
}

ErrorCode PTrace::suspend(ProcessThreadId const &ptid) {
  pid_t pid;

  if (!ptid.valid())
    return kErrorInvalidArgument;

  if (!(ptid.tid <= kAnyThreadId)) {
    pid = ptid.tid;
  } else {
    pid = ptid.pid;
  }

  if (kill(pid, SIGSTOP) < 0)
    return Platform::TranslateError();

  return kSuccess;
}

ErrorCode PTrace::step(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                       int signal, Address const &address) {
  pid_t pid;

  if (!ptid.valid())
    return kErrorInvalidArgument;

  if (!(ptid.tid <= kAnyThreadId)) {
    pid = ptid.tid;
  } else {
    pid = ptid.pid;
  }

  //
  // Continuation from address?
  //
  if (address.valid()) {
    Architecture::CPUState state;
    ErrorCode error = readCPUState(ptid, pinfo, state);
    if (error != kSuccess)
      return error;

    state.setPC(address);

    error = writeCPUState(ptid, pinfo, state);
    if (error != kSuccess)
      return error;
  }

  if (wrapPtrace(PT_STEP, pid, nullptr, signal) < 0)
    return Platform::TranslateError();

  return kSuccess;
}

ErrorCode PTrace::resume(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                         int signal, Address const &address) {
  pid_t pid;
  caddr_t addr = (caddr_t)1;

  if (!ptid.valid())
    return kErrorInvalidArgument;


  if (!(ptid.tid <= kAnyThreadId)) {
    pid = ptid.tid;
  } else {
    pid = ptid.pid;
  }

  //
  // Continuation from address?
  //
  if (address.valid()) {
    addr = (caddr_t)address.value();
  }

  if (wrapPtrace(PT_SYSCALL, pid, addr, signal) < 0)
    return Platform::TranslateError();

  return kSuccess;
}

ErrorCode PTrace::getLwpInfo(ProcessThreadId const &ptid, struct ptrace_lwpinfo *lwpinfo) {
  pid_t pid;

  if (!ptid.valid())
    return kErrorInvalidArgument;

  if (!(ptid.tid <= kAnyThreadId)) {
    pid = ptid.tid;
  } else {
    pid = ptid.pid;
  }

  if (wrapPtrace(PT_LWPINFO, pid, lwpinfo, sizeof(struct ptrace_lwpinfo)) < 0)
    return Platform::TranslateError();

  return kSuccess;
}

ErrorCode PTrace::getSigInfo(ProcessThreadId const &ptid, siginfo_t &si) {
  struct ptrace_lwpinfo lwpinfo;
  pid_t pid;

  if (!ptid.valid())
    return kErrorInvalidArgument;

  if (!(ptid.tid <= kAnyThreadId)) {
    pid = ptid.tid;
  } else {
    pid = ptid.pid;
  }

  if (wrapPtrace(PT_LWPINFO, pid, &lwpinfo, sizeof lwpinfo) < 0)
    return Platform::TranslateError();

  si = lwpinfo.pl_siginfo;
  return kSuccess;
}
}
}
}
