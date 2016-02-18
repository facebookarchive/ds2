//
// Copyright (c) 2014-present, Facebook, Inc.
// Copyright (c) 2015, Jakub Klama <jakub@ixsystems.com>
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#define __DS2_LOG_CLASS_NAME__ "PTrace"

#include "DebugServer2/Host/Darwin/PTrace.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"

#include <sys/ptrace.h>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <limits>

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace Darwin {

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

  return kSuccess; // kErrorUnsupported;
}

ErrorCode PTrace::attach(ProcessId pid) {
  if (pid <= kAnyProcessId)
    return kErrorProcessNotFound;

  DS2LOG(Debug, "attaching to pid %" PRIu64, (uint64_t)pid);

  if (wrapPtrace(PT_ATTACH, pid, nullptr, nullptr) < 0) {
    ErrorCode error = Platform::TranslateError();
    DS2LOG(Error, "Unable to attach: %d", error);
    return error;
  }

  return kSuccess;
}

ErrorCode PTrace::detach(ProcessId pid) {
  if (pid <= kAnyProcessId)
    return kErrorProcessNotFound;

  DS2LOG(Debug, "detaching from pid %" PRIu64, (uint64_t)pid);

  if (wrapPtrace(PT_DETACH, pid, nullptr, nullptr) < 0)
    return Platform::TranslateError();

  return kSuccess;
}

ErrorCode PTrace::kill(ProcessThreadId const &ptid, int signal) {
  if (!ptid.valid())
    return kErrorInvalidArgument;

  int rc;
  if (!(ptid.tid <= kAnyThreadId)) {
    DS2BUG("not implemented");
  } else {
    rc = ::kill(ptid.pid, signal);
  }

  if (rc < 0)
    return Platform::TranslateError();

  return kSuccess;
}

ErrorCode PTrace::readString(ProcessThreadId const &ptid,
                             Address const &address, std::string &str,
                             size_t length, size_t *count) {
  char buf[length];
  ErrorCode err = readMemory(ptid, address, buf, length, count);
  if (err != kSuccess)
    return err;

  if (strnlen(buf, length) == length)
    return kErrorNameTooLong;

  str = std::string(buf);
  return kSuccess;
}

ErrorCode PTrace::readMemory(ProcessThreadId const &ptid,
                             Address const &address, void *buffer,
                             size_t length, size_t *count) {
  DS2BUG("not implemented");
  return kErrorUnsupported;
}

ErrorCode PTrace::writeMemory(ProcessThreadId const &ptid,
                              Address const &address, void const *buffer,
                              size_t length, size_t *count) {
  DS2BUG("not implemented");
  return kErrorUnsupported;
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

  // (caddr_t)1 indicate that execution is to pick up where it left off.
  if (wrapPtrace(PT_STEP, pid, (caddr_t)1, signal) < 0)
    return Platform::TranslateError();

  return kSuccess;
}

ErrorCode PTrace::resume(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                         int signal, Address const &address) {
  pid_t pid;
  caddr_t addr;

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
  } else {
    // (caddr_t)1 indicate that execution is to pick up where it left off.
    addr = (caddr_t)1;
  }

  if (wrapPtrace(PT_CONTINUE, pid, addr, signal) < 0)
    return Platform::TranslateError();

  return kSuccess;
}

ErrorCode PTrace::getSigInfo(ProcessThreadId const &ptid, siginfo_t &si) {
  return kErrorUnsupported;
}
}
}
}
