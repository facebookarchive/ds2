//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Host_POSIX_Platform_h
#define __DebugServer2_Host_POSIX_Platform_h

#ifndef __DebugServer2_Host_Platform_h
#error "You shall not include this file directly."
#endif

#include <string>

namespace ds2 {
namespace Host {
namespace POSIX {

class Platform {
public:
  static void Initialize();

public:
  static CPUType GetCPUType();
  static CPUSubType GetCPUSubType();

public:
  static Endian GetEndian();

public:
  static size_t GetPointerSize();

public:
  static char const *GetHostName(bool fqdn = false);

public:
  static char const *GetOSTypeName();
  static char const *GetOSVendorName();
  static char const *GetOSVersion();
  static char const *GetOSBuild();
  static char const *GetOSKernelPath();

public:
  static bool GetUserName(UserId const &uid, std::string &name);
  static bool GetGroupName(GroupId const &gid, std::string &name);

public:
  static int OpenFile(std::string const &path, uint32_t flags, uint32_t mode);
  static bool CloseFile(int fd);
  static bool IsFilePresent(std::string const &path);
  static char const *GetWorkingDirectory();

public:
  static ProcessId GetCurrentProcessId();
};
}
}
}

#endif // !__DebugServer2_Host_POSIX_Platform_h
