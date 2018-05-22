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
#include "DebugServer2/Base.h"
#include "DebugServer2/Host/Windows/ExtraWrappers.h"
#include "DebugServer2/Types.h"
#include "DebugServer2/Utils/Log.h"
#include "DebugServer2/Utils/String.h"

#include <cstdio>
#include <lmcons.h>
#include <psapi.h>
#include <shlwapi.h>
#include <vector>
#include <windows.h>
#include <winsock2.h>

namespace ds2 {
namespace Host {

void Platform::Initialize() {
  // Disable buffering on stdout (where we print logs). When running on
  // Windows, output seems to be block-buffered, which is a problem if we want
  // to see output as it gets produced.
  setvbuf(stdout, nullptr, _IONBF, 0);

  // Initialize the socket subsystem.
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);
}

size_t Platform::GetPageSize() {
  static size_t sPageSize = 0;

  if (sPageSize == 0) {
    SYSTEM_INFO info;
    ::GetSystemInfo(&info);
    sPageSize = info.dwPageSize;
  }

  return sPageSize;
}

const char *Platform::GetHostName(bool fqdn) {
  static char hostname[255] = {'\0'};

  if (hostname[0] == '\0') {
    int rc;

    rc = ::gethostname(hostname, sizeof(hostname));
    if (rc == SOCKET_ERROR) {
      hostname[0] = '\0';
      return nullptr;
    }

    if (fqdn) {
      // TODO(sas): Get FQDN here.
    }
  }

  return hostname;
}

char const *Platform::GetOSTypeName() { return "windows"; }

char const *Platform::GetOSVendorName() { return "pc"; }

char const *Platform::GetOSVersion() {
  static char versionStr[32] = {'\0'};

  if (versionStr[0] == '\0') {
    OSVERSIONINFO version;

    version.dwOSVersionInfoSize = sizeof(version);
    if (!::GetVersionEx(&version)) {
      return versionStr;
    }

    ds2::Utils::SNPrintf(versionStr, sizeof(versionStr), "%lu.%lu",
                         version.dwMajorVersion, version.dwMinorVersion);
  }

  return versionStr;
}

char const *Platform::GetOSBuild() {
  static char buildStr[32] = {'\0'};

  if (buildStr[0] == '\0') {
    OSVERSIONINFO version;

    version.dwOSVersionInfoSize = sizeof(version);
    if (!::GetVersionEx(&version)) {
      return buildStr;
    }

    ds2::Utils::SNPrintf(buildStr, sizeof(buildStr), "%lu",
                         version.dwBuildNumber);
  }

  return buildStr;
}

char const *Platform::GetOSKernelPath() {
  static std::string kernelPath;

  if (kernelPath.size() == 0) {
    WCHAR wideKernelPath[MAX_PATH];
    UINT rc;

    rc = ::GetWindowsDirectoryW(wideKernelPath, sizeof(wideKernelPath));
    if (rc == 0 || rc >= sizeof(wideKernelPath))
      goto error;

    kernelPath = ds2::Utils::WideToNarrowString(wideKernelPath);
    kernelPath.append("\\System32\\ntoskrnl.exe");
  }

error:
  return kernelPath.c_str();
}

bool Platform::GetUserName(UserId const &uid, std::string &name) {
  WCHAR nameStr[UNLEN + 1];
  WCHAR domainStr[UNLEN + 1]; // unused
  DWORD size = UNLEN + 1;
  SID_NAME_USE nameUse; // unused
  int rc;

  rc = ::LookupAccountSidW(nullptr, uid, nameStr, &size, domainStr, &size,
                           &nameUse);
  if (rc == 0)
    return false;

  name = ds2::Utils::WideToNarrowString(nameStr);
  return true;
}

bool Platform::GetGroupName(UserId const &gid, std::string &name) {
  return GetUserName(gid, name);
}

int Platform::OpenFile(std::string const &path, uint32_t flags, uint32_t mode) {
  return -1;
}

bool Platform::CloseFile(int fd) { return false; }

bool Platform::IsFilePresent(std::string const &path) {
  // BOOL and bool are not the same type on windows. Specify `== TRUE` to
  // silence a warning.
  WIN32_FILE_ATTRIBUTE_DATA attributes; // unused
  return ::GetFileAttributesExW(ds2::Utils::NarrowToWideString(path).c_str(),
                                GetFileExInfoStandard, &attributes) == TRUE;
}

std::string Platform::GetWorkingDirectory() {
  DWORD pathSize = ::GetCurrentDirectoryW(0, nullptr);

  std::vector<WCHAR> pathStorage(pathSize);
  ::GetCurrentDirectoryW(pathSize, pathStorage.data());

  return ds2::Utils::WideToNarrowString(std::wstring(pathStorage.data()));
}

bool Platform::SetWorkingDirectory(std::string const &directory) {
  return ::SetCurrentDirectoryW(
             ds2::Utils::NarrowToWideString(directory).c_str()) == TRUE;
}

ds2::ProcessId Platform::GetCurrentProcessId() {
  return ::GetCurrentProcessId();
}

const char *Platform::GetSelfExecutablePath() {
  static std::string filename;

  if (filename.size() == 0) {
    WCHAR filenameStr[MAX_PATH];
    DWORD rc;

    rc = ::GetModuleFileNameW(nullptr, filenameStr, sizeof(filenameStr));
    if (rc == 0 || rc >= sizeof(filenameStr))
      goto error;

    filename = ds2::Utils::WideToNarrowString(filenameStr);
  }

error:
  return filename.c_str();
}

bool Platform::GetCurrentEnvironment(EnvironmentBlock &env) {
  WCHAR *envStrings = GetEnvironmentStringsW();
  WCHAR *oldEnvStrings = envStrings;
  if (envStrings == nullptr)
    return false;

  while (*envStrings) {
    std::string envStr;
    envStr = ds2::Utils::WideToNarrowString(envStrings);

    size_t equal = envStr.find('=');
    DS2ASSERT(equal != std::string::npos);

    // Some environment values can start with '=' for MS-DOS compatibility.
    // Ignore these values.
    if (equal != 0)
      env[envStr.substr(0, equal)] = envStr.substr(equal + 1);

    envStrings += wcslen(envStrings) + 1;
  }

  FreeEnvironmentStringsW(oldEnvStrings);

  return true;
}

ErrorCode Platform::TranslateError(DWORD error) {
  switch (error) {
  case ERROR_FILE_NOT_FOUND:
  case ERROR_PATH_NOT_FOUND:
    return ds2::kErrorNotFound;
  case ERROR_TOO_MANY_OPEN_FILES:
    return ds2::kErrorTooManySystemFiles;
  case ERROR_ACCESS_DENIED:
    return ds2::kErrorAccessDenied;
  case ERROR_INVALID_HANDLE:
    return ds2::kErrorInvalidHandle;
  case ERROR_NOT_ENOUGH_MEMORY:
  case ERROR_OUTOFMEMORY:
    return ds2::kErrorNoMemory;
  case ERROR_WRITE_PROTECT:
    return ds2::kErrorNotWriteable;
  case ERROR_READ_FAULT:
  case ERROR_WRITE_FAULT:
  case ERROR_INVALID_ADDRESS:
  case ERROR_NOACCESS:
    return ds2::kErrorInvalidAddress;
  case ERROR_NOT_SUPPORTED:
    return ds2::kErrorUnsupported;
  case ERROR_FILE_EXISTS:
    return ds2::kErrorAlreadyExist;
  case ERROR_INVALID_PARAMETER:
    return ds2::kErrorInvalidArgument;
  case ERROR_BAD_EXE_FORMAT:
    return ds2::kErrorInvalidArgument;
  case ERROR_PARTIAL_COPY:
    return ds2::kErrorNoSpace;
  default:
    DS2BUG("unknown error code: %#lx", error);
  }
}

ErrorCode Platform::TranslateError() { return TranslateError(GetLastError()); }

bool Platform::GetProcessInfo(ProcessId pid, ProcessInfo &info) {
  HANDLE processHandle;
  BOOL rc;

  info.clear();

  info.pid = pid;

  processHandle =
      OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
  if (processHandle == nullptr) {
    goto error;
  }

  // Get process name.
  {
    HMODULE firstModuleHandle;
    DWORD bytesNeeded;
    WCHAR processName[MAX_PATH];

    rc = EnumProcessModules(processHandle, &firstModuleHandle,
                            sizeof(firstModuleHandle), &bytesNeeded);
    if (!rc)
      goto error;

    rc = GetModuleBaseNameW(processHandle, firstModuleHandle, processName,
                            sizeof(processName));
    if (!rc)
      goto error;

    info.name = ds2::Utils::WideToNarrowString(processName);
  }

  // Get process user ID.
  {
    HANDLE processToken;
    std::vector<char> userInfoBuffer;
    PTOKEN_USER userInfo;
    DWORD bytesNeeded;

    rc = OpenProcessToken(processHandle, TOKEN_QUERY, &processToken);
    if (!rc)
      goto error;

    GetTokenInformation(processToken, TokenUser, userInfoBuffer.data(),
                        userInfoBuffer.size(), &bytesNeeded);

    userInfoBuffer.resize(bytesNeeded);

    rc = GetTokenInformation(processToken, TokenUser, userInfoBuffer.data(),
                             userInfoBuffer.size(), &bytesNeeded);
    if (!rc) {
      CloseHandle(processToken);
      goto error;
    }

    userInfo = reinterpret_cast<PTOKEN_USER>(userInfoBuffer.data());

    DWORD size = GetLengthSid(userInfo->User.Sid);
    info.realUid = malloc(size);
    CopySid(size, info.realUid, userInfo->User.Sid);

    CloseHandle(processToken);
  }

  // TODO(sas): Fetch the process group ID. This looks like it's gonna
  // require some additional work as a process on Windows doesn't have
  // a single group but a list of group tokens instead.

  CloseHandle(processHandle);

  return true;

error:
  if (processHandle != nullptr)
    CloseHandle(processHandle);
  return false;
}

void Platform::EnumerateProcesses(
    bool allUsers, UserId const &uid,
    std::function<void(ProcessInfo const &info)> const &cb) {
  std::vector<DWORD> processes;
  DWORD bytesReturned;

  processes.resize(32);

  do {
    processes.resize(2 * processes.size());
    int rc = EnumProcesses(processes.data(), processes.size() * sizeof(DWORD),
                           &bytesReturned);
    if (!rc)
      return;
  } while (bytesReturned == processes.size() * sizeof(DWORD));
  processes.resize(bytesReturned / sizeof(DWORD));

  for (auto const &e : processes) {
    ProcessInfo info;

    if (!GetProcessInfo(e, info))
      continue;

    cb(info);
  }
}

std::string Platform::GetThreadName(ProcessId pid, ThreadId tid) {
  // Note(sas): There is no thread name concept on Windows.
  // http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx describes a
  // handshake between a process and VS to transmit thread names so we
  // might end up using something similar for the inferior.
  return "";
}
} // namespace Host
} // namespace ds2
