//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Host_Linux_PTrace_h
#define __DebugServer2_Host_Linux_PTrace_h

#include "DebugServer2/Host/POSIX/PTrace.h"
#include "DebugServer2/Support/Stringify.h"
#include "DebugServer2/Utils/Log.h"

#include <sys/ptrace.h>

using ds2::Support::Stringify;

namespace ds2 {
namespace Host {
namespace Linux {

struct PTracePrivateData;

class PTrace : public POSIX::PTrace {
public:
  PTrace();
  ~PTrace() override;

public:
  ErrorCode wait(ProcessThreadId const &ptid, int *status = nullptr) override;

public:
  ErrorCode traceMe(bool disableASLR) override;
  ErrorCode traceThat(ProcessId pid) override;

public:
  ErrorCode attach(ProcessId pid) override;
  ErrorCode detach(ProcessId pid) override;

public:
  ErrorCode kill(ProcessThreadId const &ptid, int signal) override;

protected:
  virtual ErrorCode readBytes(ProcessThreadId const &ptid,
                              Address const &address, void *buffer,
                              size_t length, size_t *count, bool nullTerm);

public:
  ErrorCode readString(ProcessThreadId const &ptid, Address const &address,
                       std::string &str, size_t length,
                       size_t *count = nullptr) override;
  ErrorCode readMemory(ProcessThreadId const &ptid, Address const &address,
                       void *buffer, size_t length,
                       size_t *count = nullptr) override;
  ErrorCode writeMemory(ProcessThreadId const &ptid, Address const &address,
                        void const *buffer, size_t length,
                        size_t *count = nullptr) override;

public:
  ErrorCode readCPUState(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                         Architecture::CPUState &state) override;
  ErrorCode writeCPUState(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                          Architecture::CPUState const &state) override;

public:
  ErrorCode suspend(ProcessThreadId const &ptid) override;

public:
  ErrorCode step(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                 int signal = 0, Address const &address = Address()) override;
  ErrorCode resume(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                   int signal = 0, Address const &address = Address()) override;

public:
  ErrorCode getEventPid(ProcessThreadId const &ptid, ProcessId &epid) override;

public:
  ErrorCode getSigInfo(ProcessThreadId const &ptid, siginfo_t &si) override;

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
    DS2LOG(Debug, "running ptrace command %s on pid %d",
           Stringify::Ptrace(request, false), pid);
    return ::ptrace(static_cast<ptrace_request_t>(request), pid,
                    (void *)(uintptr_t)addr, (void *)(uintptr_t)data);
  }

public:
  PTracePrivateData *_privateData;
};
}
}
}

#endif // !__DebugServer2_Host_Linux_PTrace_h
