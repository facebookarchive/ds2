//
// Copyright (c) 2015, Jakub Klama <jakub@ixsystems.com>
// Copyright (c) 2015, Corentin Derbois <cderbois@gmail.com>
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/Darwin/LibProc.h"
#include "DebugServer2/Host/Darwin/Mach.h"
#include "DebugServer2/Target/ThreadBase.h"
#include "DebugServer2/Utils/Log.h"

#include <cstdlib>
#include <libproc.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach/task_info.h>
#include <mach/thread_info.h>
#include <stdlib.h>
#include <string>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/user.h>
#include <util.h>

namespace ds2 {
namespace Host {
namespace Darwin {

bool LibProc::GetProcessInfo(ProcessId pid, ProcessInfo &info) {
  struct proc_taskallinfo ti;
  int res = proc_pidinfo(pid, PROC_PIDTASKALLINFO, 0, &ti, sizeof(ti));
  if (res <= 0)
    return false;

  info.pid = pid;
  info.parentPid = ti.pbsd.pbi_ppid;
  info.realUid = ti.pbsd.pbi_ruid;
  info.effectiveUid = ti.pbsd.pbi_uid;
  info.realGid = ti.pbsd.pbi_rgid;
  info.effectiveGid = ti.pbsd.pbi_svgid;
  info.name = std::string(ti.pbsd.pbi_comm);

  return true;
}

void LibProc::EnumerateProcesses(
    bool allUsers, UserId const &uid,
    std::function<void(pid_t pid, uid_t uid)> const &cb) {
  DS2BUG("not implemented");
}

std::string LibProc::GetThreadName(ProcessThreadId const &ptid) {
  static const std::string defaultName = "<unknown>";
  thread_identifier_info_data_t threadId;
  struct proc_threadinfo ti;
  Mach mach;

  ErrorCode error = mach.getThreadIdentifierInfo(ptid, &threadId);
  if (error != kSuccess) {
    return defaultName;
  }

  int res = proc_pidinfo(ptid.pid, PROC_PIDTHREADINFO, threadId.thread_handle,
                         &ti, sizeof(ti));
  if (res <= 0) {
    return defaultName;
  }

  return ti.pth_name;
}

const char *LibProc::GetExecutablePath(ProcessId pid) { return "unknown"; }
} // namespace Darwin
} // namespace Host
} // namespace ds2
