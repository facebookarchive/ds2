//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Host_FreeBSD_PTrace_h
#define __DebugServer2_Host_FreeBSD_PTrace_h

#include <sys/ptrace.h>

#include "DebugServer2/Host/POSIX/PTrace.h"

namespace ds2 {
namespace Host {
namespace FreeBSD {

struct PTracePrivateData;

class PTrace : public POSIX::PTrace {
public:
  PTrace();
  virtual ~PTrace();

public:
  virtual ErrorCode traceMe(bool disableASLR);
  virtual ErrorCode traceThat(ProcessId pid);

public:
  virtual ErrorCode attach(ProcessId pid);
  virtual ErrorCode detach(ProcessId pid);

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
  virtual ErrorCode suspend(ProcessThreadId const &ptid);

public:
  virtual ErrorCode step(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                         int signal = 0, Address const &address = Address());
  virtual ErrorCode resume(ProcessThreadId const &ptid,
                           ProcessInfo const &pinfo, int signal = 0,
                           Address const &address = Address());

public:
  virtual ErrorCode getLwpInfo(ProcessThreadId const &ptid,
                               struct ptrace_lwpinfo *lwpinfo);

public:
  virtual ErrorCode getSigInfo(ProcessThreadId const &ptid, siginfo_t &si);

protected:
  void initCPUState(ProcessId pid);
  void doneCPUState();

protected:
  template <typename CommandType, typename AddrType, typename DataType>
  long wrapPtrace(CommandType request, pid_t pid, AddrType addr,
                  DataType data) {
    typedef int ptrace_request_t;
    return ::ptrace(static_cast<ptrace_request_t>(request), pid,
                    (char *)(uintptr_t)addr, (int)(uintptr_t)data);
  }

public:
  PTracePrivateData *_privateData;
};
}
}
}

#endif // !__DebugServer2_Host_FreeBSD_PTrace_h
