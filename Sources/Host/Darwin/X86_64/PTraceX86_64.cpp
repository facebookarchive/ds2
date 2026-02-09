// Copyright (c) Meta Platforms, Inc. and affiliates.
// Copyright (c) 2015, Jakub Klama <jakub@ixsystems.com>
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#include "DebugServer2/Host/Darwin/PTrace.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"

#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/user.h>

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace Darwin {

ErrorCode PTrace::readCPUState(ProcessThreadId const &ptid,
                               ProcessInfo const &pinfo,
                               Architecture::CPUState &state) {
  DS2BUG("impossible to use ptrace to %s on Darwin", __FUNCTION__);
}

ErrorCode PTrace::writeCPUState(ProcessThreadId const &ptid,
                                ProcessInfo const &pinfo,
                                Architecture::CPUState const &state) {
  DS2BUG("impossible to use ptrace to %s on Darwin", __FUNCTION__);
}
} // namespace Darwin
} // namespace Host
} // namespace ds2
