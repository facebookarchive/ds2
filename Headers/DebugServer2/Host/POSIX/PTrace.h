//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Host_POSIX_PTrace_h
#define __DebugServer2_Host_POSIX_PTrace_h

#include "DebugServer2/Architecture/CPUState.h"

#include <csignal>

namespace ds2 {
namespace Host {
namespace POSIX {

class PTrace {
public:
  PTrace();
  virtual ~PTrace();

public:
  virtual ErrorCode wait(ProcessThreadId const &ptid, int *status = nullptr);

public:
  virtual ErrorCode traceMe(bool disableASLR) = 0;
  virtual ErrorCode traceThat(ProcessId pid) = 0;

public:
  virtual ErrorCode attach(ProcessId pid) = 0;
  virtual ErrorCode detach(ProcessId pid) = 0;

public:
  virtual ErrorCode kill(ProcessThreadId const &ptid, int signal) = 0;

public:
  virtual ErrorCode readString(ProcessThreadId const &ptid,
                               Address const &address, std::string &str,
                               size_t length, size_t *nread = nullptr) = 0;
  virtual ErrorCode readMemory(ProcessThreadId const &ptid,
                               Address const &address, void *buffer,
                               size_t length, size_t *nread = nullptr) = 0;
  virtual ErrorCode writeMemory(ProcessThreadId const &ptid,
                                Address const &address, void const *buffer,
                                size_t length, size_t *nwritten = nullptr) = 0;

public:
  virtual ErrorCode readCPUState(ProcessThreadId const &ptid,
                                 ProcessInfo const &info,
                                 Architecture::CPUState &state) = 0;
  virtual ErrorCode writeCPUState(ProcessThreadId const &ptid,
                                  ProcessInfo const &info,
                                  Architecture::CPUState const &state) = 0;

public:
  virtual ErrorCode suspend(ProcessThreadId const &ptid) = 0;

public:
  virtual ErrorCode step(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                         int signal = 0,
                         Address const &address = Address()) = 0;
  virtual ErrorCode resume(ProcessThreadId const &ptid,
                           ProcessInfo const &pinfo, int signal = 0,
                           Address const &address = Address()) = 0;

public:
  virtual ErrorCode getEventPid(ProcessThreadId const &ptid, ProcessId &pid);

public:
  virtual ErrorCode getSigInfo(ProcessThreadId const &ptid, siginfo_t &si) = 0;

public:
  virtual ErrorCode execute(ProcessThreadId const &ptid,
                            ProcessInfo const &pinfo, void const *code,
                            size_t length, uint64_t &result);
};
}
}
}

#endif // !__DebugServer2_Host_POSIX_PTrace_h
