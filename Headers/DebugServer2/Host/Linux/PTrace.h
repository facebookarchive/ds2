//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Host_Linux_PTrace_h
#define __DebugServer2_Host_Linux_PTrace_h

#include <sys/ptrace.h>

#include "DebugServer2/Host/POSIX/PTrace.h"

namespace ds2 {
namespace Host {
namespace Linux {

struct PTracePrivateData;

class PTrace : public POSIX::PTrace {
public:
  PTrace();
  virtual ~PTrace();

public:
  virtual ErrorCode wait(ProcessThreadId const &ptid, bool hang = true,
                         int *status = nullptr);

public:
  virtual ErrorCode traceMe(bool disableASLR);
  virtual ErrorCode traceThat(ProcessId pid);

public:
  virtual ErrorCode attach(ProcessId pid);
  virtual ErrorCode detach(ProcessId pid);

public:
  virtual ErrorCode kill(ProcessThreadId const &ptid, int signal);

public:
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
  virtual ErrorCode suspend(ProcessThreadId const &ptid);

public:
  virtual ErrorCode step(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                         int signal = 0, Address const &address = Address());
  virtual ErrorCode resume(ProcessThreadId const &ptid,
                           ProcessInfo const &pinfo, int signal = 0,
                           Address const &address = Address());

public:
  virtual ErrorCode getEventPid(ProcessThreadId const &ptid, ProcessId &pid);

public:
  virtual ErrorCode getSigInfo(ProcessThreadId const &ptid, siginfo_t &si);

protected:
  virtual ErrorCode readRegisterSet(ProcessThreadId const &ptid, int regSetCode,
                                    void *buffer, size_t length);
  virtual ErrorCode writeRegisterSet(ProcessThreadId const &ptid,
                                     int regSetCode, void const *buffer,
                                     size_t length);

protected:
  void initCPUState(ProcessId pid);
  void doneCPUState();

protected:
  template <typename CommandType, typename AddrType, typename DataType>
  long wrapPtrace(CommandType request, pid_t pid, AddrType addr,
                  DataType data) {
#if defined(__ANDROID__)
    typedef int ptrace_request_t;
#else
    typedef enum __ptrace_request ptrace_request_t;
#endif
    return ::ptrace(static_cast<ptrace_request_t>(request), pid,
                    (void *)(uintptr_t) addr, (void *)(uintptr_t) data);
  }

public:
  PTracePrivateData *_privateData;
};
}
}
}

#endif // !__DebugServer2_Host_Linux_PTrace_h
