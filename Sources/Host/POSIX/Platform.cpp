//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Base.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <grp.h>
#include <limits.h>
#include <netdb.h>
#include <pwd.h>
#include <string>
#include <unistd.h>

#if defined(OS_FREEBSD) || defined(OS_DARWIN)
#include <sys/types.h>
#include <sys/socket.h>

extern char **environ;
#endif

// TODO HAVE_ENDIAN_H, HAVE_SYS_ENDIAN_H
#if defined(OS_LINUX)
#include <endian.h>
#elif defined(OS_DARWIN)
#include <machine/endian.h>
#else
#include <sys/endian.h>
#endif

namespace ds2 {
namespace Host {
namespace POSIX {

void Platform::Initialize() {
  // Nothing to do here.
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

char const *Platform::GetOSTypeName() { return "posix"; }

char const *Platform::GetOSVendorName() { return "unknown"; }

char const *Platform::GetOSVersion() { return nullptr; }

char const *Platform::GetOSBuild() { return nullptr; }

char const *Platform::GetOSKernelPath() { return nullptr; }

char const *Platform::GetHostName(bool fqdn) {
  static std::string sHostName;

  if (sHostName.empty()) {
    constexpr int kHostNameMax = 255;
    char hostName[kHostNameMax + 1];
    struct addrinfo hints, *info;
    int rc;

    rc = ::gethostname(hostName, kHostNameMax);
    if (rc != 0)
      goto end;

    sHostName = hostName;

    if (fqdn) {
      ::memset(&hints, 0, sizeof hints);
      hints.ai_family = AF_UNSPEC;
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_flags = AI_CANONNAME;

      rc = getaddrinfo(hostName, "http", &hints, &info);
      if (rc != 0)
        goto end;

      sHostName = info->ai_canonname;
    }
  }

end:
  return sHostName.c_str();
}

ds2::Endian Platform::GetEndian() {
#if defined(ENDIAN_LITTLE)
  return kEndianLittle;
#elif defined(ENDIAN_LITTLE)
  return kEndianBig;
#elif defined(ENDIAN_MIDDLE)
  return kEndianPDP;
#else
  return kEndianUnknown;
#endif
}

size_t Platform::GetPointerSize() { return sizeof(void *); }

bool Platform::GetUserName(UserId const &uid, std::string &name) {
  struct passwd *pwd = ::getpwuid(uid);
  if (pwd == nullptr)
    return false;

  name = pwd->pw_name;
  return true;
}

bool Platform::GetGroupName(GroupId const &gid, std::string &name) {
  struct group *grp = ::getgrgid(gid);
  if (grp == nullptr)
    return false;

  name = grp->gr_name;
  return true;
}

int Platform::OpenFile(std::string const &path, uint32_t flags, uint32_t mode) {
  return ::open(path.c_str(), static_cast<int>(flags), mode);
}

bool Platform::CloseFile(int fd) { return close(fd); }

bool Platform::IsFilePresent(std::string const &path) {
  if (path.empty())
    return false;

  return (::access(path.c_str(), F_OK) == 0);
}

char const *Platform::GetWorkingDirectory() {
  static char buf[PATH_MAX];
  return ::getcwd(buf, sizeof buf);
}

ds2::ProcessId Platform::GetCurrentProcessId() { return ::getpid(); }

bool Platform::GetCurrentEnvironment(EnvironmentBlock &env) {
  for (int i = 0; environ[i] != nullptr; ++i) {
    char *equal = strchr(environ[i], '=');
    DS2ASSERT(equal != nullptr && equal != environ[i]);
    env[std::string(environ[i], equal)] = equal + 1;
  }

  return true;
}

ErrorCode Platform::TranslateError(int error) {
  switch (error) {
  case EAGAIN:
  case EBUSY:
    return ds2::kErrorBusy;
  case ESRCH:
    return ds2::kErrorProcessNotFound;
  case EBADF:
    return ds2::kErrorInvalidHandle;
  case EFAULT:
  case EIO:
    return ds2::kErrorInvalidAddress;
  case EPERM:
    return ds2::kErrorNoPermission;
  case EEXIST:
    return ds2::kErrorAlreadyExist;
  case EINVAL:
    return ds2::kErrorInvalidArgument;
  default:
    DS2BUG("unknown error code: %d", error);
  }
}

ErrorCode Platform::TranslateError() { return TranslateError(errno); }
}
}
}
