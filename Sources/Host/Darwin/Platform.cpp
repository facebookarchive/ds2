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
#include "DebugServer2/Utils/Log.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mach/mach.h>
#include <string>
#include <sys/utsname.h>

namespace ds2 {
namespace Host {

char const *Platform::GetOSTypeName() { return "macosx"; }

char const *Platform::GetOSVendorName() { return "apple"; }

static struct utsname const *GetCachedUTSName() {
  static struct utsname sUName = {"", "", "", "", ""};
  if (sUName.release[0] == '\0') {
    ::uname(&sUName);
  }
  return &sUName;
}

char const *Platform::GetOSVersion() {
  static char version[9] = {0};

  // macOS doesn't offer a C/C++ call to get the OS version.
  // SystemVersion.plist is used by official applications to get the version.
  // An XML parser would be nice but would add a heavy dependency to read one
  // static value.

  if (version[0] == '\0') {
    std::string line;
    std::ifstream versionFile(
        "/System/Library/CoreServices/SystemVersion.plist");

    if (!versionFile.is_open()) {
      return version;
    }

    // We are looking for:
    // <dict>
    // ...
    // <key>ProductVersion</key>
    // <string>10.10.5</string>
    // ...
    // </dict>

    while (std::getline(versionFile, line)) {
      size_t start = 0;
      size_t end = 0;
      size_t len = 0;

      if (line.find("ProductVersion") == std::string::npos)
        continue;

      if (!std::getline(versionFile, line))
        break;

      start = line.find("<string>");
      end = line.find("</string>");

      if (start == std::string::npos || end == std::string::npos)
        break;

      start += 8;

      len = (end - start) > 8 ? 8 : end - start;

      std::memcpy(version, line.substr(start, end).c_str(), len);
    }

    versionFile.close();
  }

  return version;
}

char const *Platform::GetOSBuild() { return GetCachedUTSName()->version; }

char const *Platform::GetOSKernelPath() { return nullptr; }

const char *Platform::GetSelfExecutablePath() {
  return Host::Darwin::LibProc::GetExecutablePath(getpid());
}

ErrorCode Platform::TranslateKernError(kern_return_t kret) {
  switch (kret) {
  case KERN_SUCCESS:
    return ds2::kSuccess;
  case KERN_FAILURE:
    return ds2::kErrorUnknown;
  case KERN_MEMORY_FAILURE:
  case KERN_INVALID_ADDRESS:
  case KERN_PROTECTION_FAILURE:
    return ds2::kErrorInvalidAddress;
  case KERN_NO_ACCESS:
    return ds2::kErrorNoPermission;
  case KERN_RESOURCE_SHORTAGE:
    return ds2::kErrorNoMemory;
  default:
    DS2BUG("unknown error code: %d [%s]", kret, mach_error_string(kret));
  }
}

bool Platform::GetProcessInfo(ProcessId pid, ProcessInfo &info) {
  return Host::Darwin::LibProc::GetProcessInfo(pid, info);
}

void Platform::EnumerateProcesses(
    bool allUsers, UserId const &uid,
    std::function<void(ProcessInfo const &info)> const &cb) {
  Host::Darwin::LibProc::EnumerateProcesses(allUsers, uid,
                                            [&](pid_t pid, uid_t uid) {
                                              ProcessInfo info;

                                              if (!GetProcessInfo(pid, info))
                                                return;

                                              cb(info);
                                            });
}

std::string Platform::GetThreadName(ProcessId pid, ThreadId tid) {
  return Host::Darwin::LibProc::GetThreadName(ProcessThreadId(pid, tid));
}
} // namespace Host
} // namespace ds2
