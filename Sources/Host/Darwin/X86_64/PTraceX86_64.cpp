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
