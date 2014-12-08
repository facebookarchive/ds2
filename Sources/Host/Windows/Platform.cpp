//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Base.h"
#include "DebugServer2/Host/Platform.h"

#include <lmcons.h>
#include <psapi.h>
// When building this with clang, building in C++ mode will require the
// declaration of IUnknown (which is a builtin type on MSVC).  Use the C
// interfaces, as we only need the Path APIs.
#define CINTERFACE
#include <shlwapi.h>
#include <stdio.h>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <Winsock2.h>

using ds2::Host::Windows::Platform;

void Platform::Initialize() {
  // Disable buffering on standard streams. When running on Windows,
  // output seems to be block-buffered, which is a problem if we want
  // to see output as it gets produced.
  setvbuf(stdout, nullptr, _IONBF, 0);
  setvbuf(stderr, nullptr, _IONBF, 0);

  // Initialize the socket subsystem.
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);
}

ds2::CPUType Platform::GetCPUType() {
#if defined(ARCH_ARM) || defined(ARCH_ARM64)
  return (sizeof(void *) == 8) ? kCPUTypeARM64 : kCPUTypeARM;
#elif defined(ARCH_X86) || defined(ARCH_X86_64)
  return (sizeof(void *) == 8) ? kCPUTypeX86_64 : kCPUTypeI386;
#else
#error "Architecture not supported."
#endif
}

ds2::CPUSubType Platform::GetCPUSubType() {
#if defined(ARCH_ARM)
#if defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) ||                     \
    defined(__ARM_ARCH_7R__)
  return kCPUSubTypeARM_V7;
#elif defined(__ARM_ARCH_7EM__)
  return kCPUSubTypeARM_V7EM;
#elif defined(__ARM_ARCH_7M__)
  return kCPUSubTypeARM_V7M;
#endif
#endif
  return kCPUSubTypeInvalid;
}

ds2::Endian Platform::GetEndian() {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  return kEndianLittle;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  return kEndianBig;
#elif __BYTE_ORDER__ == __ORDER_PDP_ENDIAN__
  return kEndianPDP;
#else
  return kEndianUnknown;
#endif
}

size_t Platform::GetPointerSize() { return sizeof(void *); }

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

char const *Platform::GetOSVendorName() { return "unknown"; }

char const *Platform::GetOSVersion() {
  static char versionStr[32] = {'\0'};

  if (versionStr[0] == '\0') {
    DWORD version;

    version = ::GetVersion();

    ::_snprintf(versionStr, sizeof(versionStr), "%d.%d",
                (DWORD)(LOBYTE(LOWORD(version))),
                (DWORD)(HIBYTE(LOWORD(version))));
  }

  return versionStr;
}

char const *Platform::GetOSBuild() {
  static char buildStr[32] = {'\0'};

  if (buildStr[0] == '\0') {
    DWORD version;

    version = ::GetVersion();

    if (version < 0x80000000) {
      ::_snprintf(buildStr, sizeof(buildStr), "%d", (DWORD)(HIWORD(version)));
    }
  }

  return buildStr;
}

char const *Platform::GetOSKernelPath() {
  static char kernelPath[MAX_PATH] = {'\0'};

  if (kernelPath[0] == '\0') {
    UINT rc;

    rc = ::GetWindowsDirectoryA(kernelPath, sizeof(kernelPath));
    if (rc == 0 || rc >= sizeof(kernelPath)) {
      kernelPath[0] = '\0';
      return nullptr;
    }

    ::strncpy(kernelPath + rc, "\\System32\\ntoskrnl.exe",
              sizeof(kernelPath) - rc);
    kernelPath[sizeof(kernelPath) - 1] = '\0';
  }

  return kernelPath;
}

bool Platform::GetUserName(UserId const &uid, std::string &name) {
  char nameStr[UNLEN];
  char domainStr[UNLEN]; // unused
  DWORD size = UNLEN;
  SID_NAME_USE nameUse; // unused
  int rc;

  rc = ::LookupAccountSidA(nullptr, uid, nameStr, &size, domainStr, &size,
                           &nameUse);
  if (rc == 0)
    return false;

  name = nameStr;
  return true;
}

bool Platform::GetGroupName(UserId const &gid, std::string &name) {
  return GetUserName(gid, name);
}

bool Platform::IsFilePresent(std::string const &path) {
  return ::PathFileExistsA(path.c_str());
}

char const *Platform::GetWorkingDirectory() { return nullptr; }

ds2::ProcessId Platform::GetCurrentProcessId() {
  return ::GetCurrentProcessId();
}

bool Platform::GetProcessInfo(ProcessId pid, ProcessInfo &info) {
  HANDLE processHandle;
  BOOL rc;

  info.clear();

  info.pid = pid;

  processHandle =
      OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
  if (processHandle == nullptr)
    goto error;

  // Get process name.
  {
    HMODULE firstModuleHandle;
    DWORD bytesNeeded;
    char processName[MAX_PATH];

    rc = EnumProcessModules(processHandle, &firstModuleHandle,
                            sizeof(firstModuleHandle), &bytesNeeded);
    if (!rc)
      goto error;

    rc = GetModuleBaseNameA(processHandle, firstModuleHandle, processName,
                            sizeof(processName));
    if (!rc)
      goto error;

    info.name = processName;
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
    int rc;

    processes.resize(2 * processes.size());
    rc = EnumProcesses(processes.data(), processes.size() * sizeof(DWORD),
                       &bytesReturned);
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
  return "<NONE>";
}

const char *Platform::GetSelfExecutablePath() {
  static char filenameStr[MAX_PATH] = {'\0'};

  if (filenameStr[0] == '\0') {
    DWORD rc;

    rc = ::GetModuleFileNameA(nullptr, filenameStr, sizeof(filenameStr));
    if (rc == 0 || rc >= sizeof(filenameStr)) {
      filenameStr[0] = '\0';
      return nullptr;
    }
  }

  return filenameStr;
}
