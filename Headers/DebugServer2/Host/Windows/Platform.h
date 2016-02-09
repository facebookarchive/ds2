//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Host_Windows_Platform_h
#define __DebugServer2_Host_Windows_Platform_h

#ifndef __DebugServer2_Host_Platform_h
#error "You shall not include this file directly."
#endif

#include "DebugServer2/Types.h"

#include <functional>
#include <string>

namespace ds2 {
namespace Host {
namespace Windows {

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
  static const char *GetOSTypeName();
  static const char *GetOSVendorName();
  static const char *GetOSVersion();
  static const char *GetOSBuild();
  static const char *GetOSKernelPath();

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

public:
  static bool GetProcessInfo(ProcessId pid, ProcessInfo &info);
  static void
  EnumerateProcesses(bool allUsers, UserId const &uid,
                     std::function<void(ProcessInfo const &info)> const &cb);

public:
  static std::string GetThreadName(ProcessId pid, ThreadId tid);

public:
  static const char *GetSelfExecutablePath();
  static bool GetCurrentEnvironment(EnvironmentBlock &env);

public:
  static ErrorCode TranslateError(DWORD error);
  static ErrorCode TranslateError();

public:
  static std::wstring NarrowToWideString(std::string const &s);
  static std::string WideToNarrowString(std::wstring const &s);
};
}
}
}

#endif // !__DebugServer2_Host_Windows_Platform_h
