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
namespace Darwin {

struct PTracePrivateData;

class PTrace : public POSIX::PTrace {
public:
  ErrorCode traceThat(ProcessId pid) override;

public:
  ErrorCode kill(ProcessThreadId const &ptid, int signal) override;

public:
  ErrorCode readString(ProcessThreadId const &ptid, Address const &address,
                       std::string &str, size_t length,
                       size_t *nread = nullptr) override;
  ErrorCode readMemory(ProcessThreadId const &ptid, Address const &address,
                       void *buffer, size_t length,
                       size_t *nread = nullptr) override;
  ErrorCode writeMemory(ProcessThreadId const &ptid, Address const &address,
                        void const *buffer, size_t length,
                        size_t *nwritten = nullptr) override;

public:
  ErrorCode readCPUState(ProcessThreadId const &ptid, ProcessInfo const &info,
                         Architecture::CPUState &state) override;
  ErrorCode writeCPUState(ProcessThreadId const &ptid, ProcessInfo const &info,
                          Architecture::CPUState const &state) override;

public:
  ErrorCode suspend(ProcessThreadId const &ptid) override;

public:
  ErrorCode getSigInfo(ProcessThreadId const &ptid, siginfo_t &si) override;
};
} // namespace Darwin
} // namespace Host
} // namespace ds2
