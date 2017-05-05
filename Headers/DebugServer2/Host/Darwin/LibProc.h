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

#include "DebugServer2/Target/ThreadBase.h"
#include "DebugServer2/Types.h"

#include <cstdio>
#include <dirent.h>
#include <fcntl.h>
#include <functional>
#include <unistd.h>

#include <sys/stat.h>

namespace ds2 {
namespace Host {
namespace Darwin {

class LibProc {
public:
  static bool GetProcessInfo(ProcessId pid, ProcessInfo &info);
  static void
  EnumerateProcesses(bool allUsers, UserId const &uid,
                     std::function<void(pid_t pid, uid_t uid)> const &cb);
  static std::string GetThreadName(ProcessThreadId const &ptid);
  static const char *GetExecutablePath(ProcessId pid);
};
} // namespace Darwin
} // namespace Host
} // namespace ds2
