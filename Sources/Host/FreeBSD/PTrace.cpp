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

#include "DebugServer2/Host/FreeBSD/PTrace.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <limits>
#include <sys/thr.h>

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace FreeBSD {

ErrorCode PTrace::traceThat(ProcessId pid) {
  if (pid <= 0)
    return kErrorInvalidArgument;

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
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

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
  CHK(ptidToPid(ptid, pid));

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

ErrorCode PTrace::getLwpInfo(ProcessThreadId const &ptid,
                             struct ptrace_lwpinfo *lwpinfo) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  if (wrapPtrace(PT_LWPINFO, pid, lwpinfo, sizeof(struct ptrace_lwpinfo)) < 0)
    return Platform::TranslateError();

  return kSuccess;
}

ErrorCode PTrace::getSigInfo(ProcessThreadId const &ptid, siginfo_t &si) {
  struct ptrace_lwpinfo lwpinfo;
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  if (wrapPtrace(PT_LWPINFO, pid, &lwpinfo, sizeof lwpinfo) < 0)
    return Platform::TranslateError();

  si = lwpinfo.pl_siginfo;
  return kSuccess;
}
} // namespace FreeBSD
} // namespace Host
} // namespace ds2
