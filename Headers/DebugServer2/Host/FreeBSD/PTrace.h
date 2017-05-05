//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#pragma once

#include "DebugServer2/Architecture/CPUState.h"
#include "DebugServer2/Host/POSIX/PTrace.h"

namespace ds2 {
namespace Host {
namespace FreeBSD {

struct PTracePrivateData;

class PTrace : public POSIX::PTrace {
public:
  virtual ErrorCode traceThat(ProcessId pid);

public:
  virtual ErrorCode kill(ProcessThreadId const &ptid, int signal);

public:
  virtual ErrorCode readString(ProcessThreadId const &ptid,
                               Address const &address, std::string &str,
                               size_t length, size_t *nread = nullptr);
  virtual ErrorCode readMemory(ProcessThreadId const &ptid,
                               Address const &address, void *buffer,
                               size_t length, size_t *nread = nullptr);
  virtual ErrorCode writeMemory(ProcessThreadId const &ptid,
                                Address const &address, void const *buffer,
                                size_t length, size_t *nwritten = nullptr);

public:
  virtual ErrorCode readCPUState(ProcessThreadId const &ptid,
                                 ProcessInfo const &info,
                                 Architecture::CPUState &state);
  virtual ErrorCode writeCPUState(ProcessThreadId const &ptid,
                                  ProcessInfo const &info,
                                  Architecture::CPUState const &state);

public:
  virtual ErrorCode getLwpInfo(ProcessThreadId const &ptid,
                               struct ptrace_lwpinfo *lwpinfo);

public:
  virtual ErrorCode getSigInfo(ProcessThreadId const &ptid, siginfo_t &si);
};
} // namespace FreeBSD
} // namespace Host
} // namespace ds2
