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

#include "DebugServer2/Utils/Log.h"
#include "DebugServer2/Host/Darwin/LibProc.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach/thread_info.h>

#include <util.h>
#include <stdlib.h>
#include <string>
#include <libproc.h>

namespace ds2 {
namespace Host {
namespace Darwin {

bool LibProc::GetProcessInfo(ProcessId pid, ProcessInfo &info) {
  int res;
  struct proc_taskallinfo ti;

  res = proc_pidinfo(pid, PROC_PIDTASKALLINFO, 0, &ti, sizeof(ti));
  if (res <= 0)
    return false;

  info.pid = pid;
  info.parentPid = static_cast<ProcessId>(ti.pbsd.pbi_ppid);
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

std::string LibProc::GetThreadName(ProcessId pid, ThreadId tid) {
  return "<unknown>";
}

const char *LibProc::GetExecutablePath(ProcessId pid) { return "unknown"; }
}
}
}
