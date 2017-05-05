//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Host/Linux/ProcFS.h"
#include "DebugServer2/Utils/String.h"

#include <sys/utsname.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>

namespace ds2 {
namespace Host {

char const *Platform::GetOSTypeName() {
#if defined(PLATFORM_ANDROID)
  return "linux-android";
#elif defined(PLATFORM_TIZEN)
  return "linux-gnueabi";
#else
  return "linux";
#endif
}

char const *Platform::GetOSVendorName() {
  static std::string vendor;

  if (vendor.empty()) {
    // Use /etc/lsb-release to extract the vendor and cache it.
    FILE *fp = std::fopen("/etc/lsb-release", "r");
    if (fp != nullptr) {
      Host::Linux::ProcFS::ParseKeyValue(
          fp, 1024, '=', [&](char const *key, char const *value) -> bool {
            if (key == nullptr)
              return true;

            if (std::strcmp(key, "DISTRIB_ID") == 0) {
              vendor = value;
              // make the vendor lowercase
              std::transform(vendor.begin(), vendor.end(), vendor.begin(),
                             static_cast<int (*)(int)>(&std::tolower));
              return false; // break the loop
            }
            return true;
          });
      std::fclose(fp);
    } else {
      vendor = "unknown";
    }
  }

  return vendor.c_str();
}

static struct utsname const *GetCachedUTSName() {
  static struct utsname sUName = {"", "", "", "", "", ""};
  if (sUName.release[0] == '\0') {
    ::uname(&sUName);
  }
  return &sUName;
}

char const *Platform::GetOSVersion() { return GetCachedUTSName()->release; }

char const *Platform::GetOSBuild() {
  static char sBuild[32] = {'\0'};

  if (sBuild[0] == '\0') {
    char const *version = GetCachedUTSName()->version;
    // Linux version is returned as #BUILDNO ...
    ds2::Utils::SNPrintf(sBuild, sizeof(sBuild), "%lu",
                         std::strtoul(version + 1, nullptr, 10));
  }

  return sBuild;
}

char const *Platform::GetOSKernelPath() { return nullptr; }

const char *Platform::GetSelfExecutablePath() {
  static char path[PATH_MAX + 1] = {'\0'};

  if (path[0] == '\0') {
    Host::Linux::ProcFS::ReadLink(0, "exe", path, PATH_MAX);
  }

  return path;
}

bool Platform::GetProcessInfo(ProcessId pid, ProcessInfo &info) {
  return Host::Linux::ProcFS::ReadProcessInfo(pid, info);
}

void Platform::EnumerateProcesses(
    bool allUsers, UserId const &uid,
    std::function<void(ProcessInfo const &info)> const &cb) {
  Host::Linux::ProcFS::EnumerateProcesses(allUsers, uid,
                                          [&](pid_t pid, uid_t uid) {
                                            ProcessInfo info;

                                            if (!GetProcessInfo(pid, info))
                                              return;

                                            cb(info);
                                          });
}

std::string Platform::GetThreadName(ProcessId pid, ThreadId tid) {
  return Host::Linux::ProcFS::GetThreadName(pid, tid);
}
} // namespace Host
} // namespace ds2
