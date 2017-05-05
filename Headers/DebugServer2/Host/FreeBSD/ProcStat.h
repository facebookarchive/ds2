//
// Copyright (c) 2015, Jakub Klama <jakub@ixsystems.com>
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#pragma once

#include "DebugServer2/Support/POSIX/ELFSupport.h"
#include "DebugServer2/Types.h"

#include <cstdio>
#include <dirent.h>
#include <fcntl.h>
#include <functional>
#include <sys/proc.h>
#include <sys/stat.h>
#include <unistd.h>

namespace ds2 {
namespace Host {
namespace FreeBSD {

using ds2::Support::ELFSupport;

enum ProcState {
  kProcStateDead = 0,
  kProcStateRunning = SRUN,
  kProcStateSleeping = SSLEEP,
  kProcStateWaiting = SWAIT,
  kProcStateLock = SLOCK,
  kProcStateStopped = SSTOP,
  kProcStateZombie = SZOMB
};

class ProcStat {
public:
  static bool GetProcessInfo(ProcessId pid, ProcessInfo &info);
  static bool GetProcessMap(pid_t pid, std::vector<ds2::MemoryRegionInfo> &map);
  static bool GetThreadState(pid_t pid, pid_t tid, int &state, int &cpu);
  static bool EnumerateAuxiliaryVector(
      pid_t pid,
      std::function<void(ELFSupport::AuxiliaryVectorEntry const &)> const &cb);
  static void
  EnumerateProcesses(bool allUsers, UserId const &uid,
                     std::function<void(pid_t pid, uid_t uid)> const &cb);
  static void EnumerateThreads(pid_t pid,
                               std::function<void(pid_t tid)> const &cb);
  static std::string GetThreadName(ProcessId pid, ThreadId tid);
  static std::string GetExecutablePath(ProcessId pid);
};
} // namespace FreeBSD
} // namespace Host
} // namespace ds2
