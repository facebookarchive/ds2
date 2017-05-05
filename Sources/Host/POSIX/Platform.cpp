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
#include "DebugServer2/Utils/Log.h"

#include <cerrno>
#include <climits>
#include <cstring>
#include <fcntl.h>
#include <grp.h>
#include <netdb.h>
#include <pwd.h>
#include <string>
#include <unistd.h>

#if defined(OS_FREEBSD) || defined(OS_DARWIN)
#include <sys/socket.h>
#include <sys/types.h>

extern char **environ;
#endif

namespace ds2 {
namespace Host {

void Platform::Initialize() {
  // Nothing to do here.
}

size_t Platform::GetPageSize() {
  static size_t sPageSize = 0;

  if (sPageSize == 0) {
    sPageSize = ::sysconf(_SC_PAGESIZE);
  }

  return sPageSize;
}

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

      rc = ::getaddrinfo(hostName, "http", &hints, &info);
      if (rc != 0)
        goto end;

      sHostName = info->ai_canonname;

      ::freeaddrinfo(info);
    }
  }

end:
  return sHostName.c_str();
}

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
  return ::open(path.c_str(), flags, mode);
}

bool Platform::CloseFile(int fd) { return ::close(fd); }

bool Platform::IsFilePresent(std::string const &path) {
  if (path.empty()) {
    return false;
  }

  return (::access(path.c_str(), F_OK) == 0);
}

std::string Platform::GetWorkingDirectory() {
  char buf[PATH_MAX];
  return ::getcwd(buf, sizeof buf);
}

bool Platform::SetWorkingDirectory(std::string const &directory) {
  return ::chdir(directory.c_str()) == 0;
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
  case EINTR:
    return ds2::kErrorInterrupted;
  case EAGAIN:
  case EBUSY:
    return ds2::kErrorBusy;
  case ESRCH:
    return ds2::kErrorProcessNotFound;
  case EBADF:
    return ds2::kErrorInvalidHandle;
  case EACCES:
    return ds2::kErrorAccessDenied;
  case EFAULT:
  case EIO:
    return ds2::kErrorInvalidAddress;
  case EPERM:
  case ENOEXEC:
    return ds2::kErrorNoPermission;
  case EEXIST:
    return ds2::kErrorAlreadyExist;
  case EINVAL:
    return ds2::kErrorInvalidArgument;
  case ENOENT:
    return ds2::kErrorNotFound;
  default:
    DS2BUG("unknown error code: %d", error);
  }
}

ErrorCode Platform::TranslateError() { return TranslateError(errno); }
} // namespace Host
} // namespace ds2
