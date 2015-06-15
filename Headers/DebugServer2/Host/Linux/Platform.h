//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Host_Linux_Platform_h
#define __DebugServer2_Host_Linux_Platform_h

#ifndef __DebugServer2_Host_Platform_h
#error "You shall not include this file directly."
#endif

#include "DebugServer2/Host/POSIX/Platform.h"

#include <functional>

namespace ds2 {
namespace Host {
namespace Linux {

class Platform : public POSIX::Platform {
public:
  static char const *GetOSTypeName();
  static char const *GetOSVendorName();
  static char const *GetOSVersion();
  static char const *GetOSBuild();

public:
  static bool GetProcessInfo(ProcessId pid, ProcessInfo &info);
  static void
  EnumerateProcesses(bool allUsers, UserId const &uid,
                     std::function<void(ProcessInfo const &info)> const &cb);

public:
  static std::string GetThreadName(ProcessId pid, ThreadId tid);

public:
  static const char *GetSelfExecutablePath();
};
}
}
}

#endif // !__DebugServer2_Host_Linux_Platform_h
