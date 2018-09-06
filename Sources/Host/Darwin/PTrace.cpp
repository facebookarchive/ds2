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

#include "DebugServer2/Host/Darwin/PTrace.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <limits>
#include <sys/ptrace.h>

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace Darwin {

ErrorCode PTrace::traceThat(ProcessId pid) {
  if (pid <= 0) {
    return kErrorInvalidArgument;
  }

  return kSuccess; // kErrorUnsupported;
}

ErrorCode PTrace::kill(ProcessThreadId const &ptid, int signal) {
  if (!ptid.valid()) {
    return kErrorInvalidArgument;
  }

  int rc;
  if (!(ptid.tid <= kAnyThreadId)) {
    DS2BUG("not implemented");
  } else {
    rc = ::kill(ptid.pid, signal);
  }

  if (rc < 0) {
    return Platform::TranslateError();
  }

  return kSuccess;
}

ErrorCode PTrace::readString(ProcessThreadId const &ptid,
                             Address const &address, std::string &str,
                             size_t length, size_t *count) {
  DS2BUG("impossible to use ptrace to %s on Darwin", __FUNCTION__);
}

ErrorCode PTrace::readMemory(ProcessThreadId const &ptid,
                             Address const &address, void *buffer,
                             size_t length, size_t *count) {
  DS2BUG("impossible to use ptrace to %s on Darwin", __FUNCTION__);
}

ErrorCode PTrace::writeMemory(ProcessThreadId const &ptid,
                              Address const &address, void const *buffer,
                              size_t length, size_t *count) {
  DS2BUG("impossible to use ptrace to %s on Darwin", __FUNCTION__);
}

ErrorCode PTrace::suspend(ProcessThreadId const &ptid) {
  // TODO(sas): I don't think this will work. Not sure we can send signal to
  // individual threads on Darwin.
  return PTrace::kill(ptid, SIGSTOP);
}

ErrorCode PTrace::getSigInfo(ProcessThreadId const &ptid, siginfo_t &si) {
  return kErrorUnsupported;
}
} // namespace Darwin
} // namespace Host
} // namespace ds2
