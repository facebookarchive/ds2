//
// Copyright (c) 2015, Jakub Klama <jakub@ixsystems.com>
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Host/Darwin/LibProc.h"

#include <sys/utsname.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>

namespace ds2 {
namespace Host {
namespace Darwin {

char const *Platform::GetOSTypeName() { return "macosx"; }

char const *Platform::GetOSVendorName() { return "unknown"; }

static struct utsname const *GetCachedUTSName() {
  static struct utsname sUName = {"", "", "", "", ""};
  if (sUName.release[0] == '\0') {
    ::uname(&sUName);
  }
  return &sUName;
}

char const *Platform::GetOSVersion() { return GetCachedUTSName()->release; }

char const *Platform::GetOSBuild() { return GetCachedUTSName()->version; }

bool Platform::GetProcessInfo(ProcessId pid, ProcessInfo &info) {
  return LibProc::GetProcessInfo(pid, info);
}

void Platform::EnumerateProcesses(
    bool allUsers, UserId const &uid,
    std::function<void(ProcessInfo const &info)> const &cb) {
  LibProc::EnumerateProcesses(allUsers, uid, [&](pid_t pid, uid_t uid) {
    ProcessInfo info;

    if (!GetProcessInfo(pid, info))
      return;

    cb(info);
  });
}

std::string Platform::GetThreadName(ProcessId pid, ThreadId tid) {
  return LibProc::GetThreadName(ProcessThreadId(pid, tid));
}

const char *Platform::GetSelfExecutablePath() {
  return LibProc::GetExecutablePath(getpid());
}
}
}
}
