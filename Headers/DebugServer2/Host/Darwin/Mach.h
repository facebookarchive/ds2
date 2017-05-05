//
// Copyright (c) 2015 Corentin Derbois <cderbois@gmail.com>
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#pragma once

#include "DebugServer2/Architecture/CPUState.h"

#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach/thread_info.h>
#include <sys/types.h>

namespace ds2 {
namespace Host {
namespace Darwin {

class Mach {
public:
  virtual ~Mach() = default;

public:
  ErrorCode readMemory(ProcessThreadId const &ptid, Address const &address,
                       void *buffer, size_t length, size_t *nread = nullptr);
  ErrorCode writeMemory(ProcessThreadId const &ptid, Address const &address,
                        void const *buffer, size_t length,
                        size_t *nwritten = nullptr);

public:
  ErrorCode readCPUState(ProcessThreadId const &ptid, ProcessInfo const &info,
                         Architecture::CPUState &state);
  ErrorCode writeCPUState(ProcessThreadId const &ptid, ProcessInfo const &info,
                          Architecture::CPUState const &state);

public:
  ErrorCode suspend(ProcessThreadId const &ptid);

public:
  ErrorCode step(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                 int signal = 0, Address const &address = Address());
  ErrorCode resume(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                   int signal = 0, Address const &address = Address());

public:
  ErrorCode getProcessDylbInfo(ProcessId pid, Address &address);
  ErrorCode getProcessMemoryRegion(ProcessId pid, Address const &address,
                                   MemoryRegionInfo &info);

public:
  ErrorCode getThreadInfo(ProcessThreadId const &tid, thread_basic_info_t info);
  ErrorCode getThreadIdentifierInfo(ProcessThreadId const &tid,
                                    thread_identifier_info_data_t *threadID);

private:
  task_t getMachTask(ProcessId pid);
  thread_t getMachThread(ProcessThreadId const &tid);
};
} // namespace Darwin
} // namespace Host
} // namespace ds2
