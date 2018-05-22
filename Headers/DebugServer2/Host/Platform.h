//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#pragma once

#include "DebugServer2/Types.h"

#include <functional>
#if defined(OS_DARWIN)
#include <mach/kern_return.h>
#endif
#include <string>

namespace ds2 {
namespace Host {

class Platform {
public:
  static void Initialize();

public:
  static CPUType GetCPUType();
  static CPUSubType GetCPUSubType();
  static Endian GetEndian();
  static size_t GetPointerSize();
  static size_t GetPageSize();

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
  static std::string GetWorkingDirectory();
  static bool SetWorkingDirectory(std::string const &directory);

public:
  static ProcessId GetCurrentProcessId();
  static const char *GetSelfExecutablePath();
  static bool GetCurrentEnvironment(EnvironmentBlock &env);

public:
#if defined(OS_POSIX)
  static ErrorCode TranslateError(int error);
#elif defined(OS_WIN32)
  static ErrorCode TranslateError(DWORD error);
#endif
  static ErrorCode TranslateError();
#if defined(OS_DARWIN)
  static ErrorCode TranslateKernError(kern_return_t kret);
#endif

public:
  static bool GetProcessInfo(ProcessId pid, ProcessInfo &info);
  static void
  EnumerateProcesses(bool allUsers, UserId const &uid,
                     std::function<void(ProcessInfo const &info)> const &cb);

public:
  static std::string GetThreadName(ProcessId pid, ThreadId tid);
};
} // namespace Host
} // namespace ds2
