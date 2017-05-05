//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/File.h"
#include "DebugServer2/Host/Platform.h"

#include <fcntl.h>
#include <limits>
#include <sys/stat.h>
#include <unistd.h>

namespace ds2 {
namespace Host {

static int convertFlags(OpenFlags ds2Flags) {
  int flags = 0;

  if (ds2Flags & kOpenFlagRead)
    flags |= (ds2Flags & kOpenFlagWrite) ? O_RDWR : O_RDONLY;
  else if (ds2Flags & kOpenFlagWrite)
    flags |= O_WRONLY;

  if (ds2Flags & kOpenFlagAppend)
    flags |= O_APPEND;
  if (ds2Flags & kOpenFlagTruncate)
    flags |= O_TRUNC;
  if (ds2Flags & kOpenFlagNonBlocking)
    flags |= O_NONBLOCK;
  if (ds2Flags & kOpenFlagCreate)
    flags |= O_CREAT;
  if (ds2Flags & kOpenFlagNewOnly)
    flags |= O_EXCL;
  if (ds2Flags & kOpenFlagNoFollow)
#if defined(O_NOFOLLOW)
    flags |= O_NOFOLLOW;
#else
    return -1;
#endif
  if (ds2Flags & kOpenFlagCloseOnExec)
    flags |= O_CLOEXEC;

  return flags;
}

File::File(std::string const &path, OpenFlags flags, uint32_t mode) {
  int posixFlags = convertFlags(flags);
  if (posixFlags < 0) {
    _lastError = kErrorInvalidArgument;
    return;
  }

  _fd = ::open(path.c_str(), posixFlags, mode);
  _lastError = (_fd < 0) ? Platform::TranslateError() : kSuccess;
}

File::~File() {
  if (valid()) {
    ::close(_fd);
  }
}

ErrorCode File::pread(ByteVector &buf, uint64_t &count, uint64_t offset) {
  if (!valid()) {
    return _lastError = kErrorInvalidHandle;
  }

  if (offset > std::numeric_limits<off_t>::max()) {
    return _lastError = kErrorInvalidArgument;
  }

  auto offArg = static_cast<off_t>(offset);
  auto countArg = static_cast<size_t>(count);

  buf.resize(countArg);

  ssize_t nRead = ::pread(_fd, buf.data(), countArg, offArg);

  if (nRead < 0) {
    return _lastError = Platform::TranslateError();
  }

  buf.resize(nRead);
  count = static_cast<uint64_t>(nRead);

  return _lastError = kSuccess;
}

ErrorCode File::pwrite(ByteVector const &buf, uint64_t &count,
                       uint64_t offset) {
  DS2ASSERT(count > 0);

  if (!valid()) {
    return _lastError = kErrorInvalidHandle;
  }

  if (offset > std::numeric_limits<off_t>::max()) {
    return _lastError = kErrorInvalidArgument;
  }

  auto offArg = static_cast<off_t>(offset);
  auto countArg = static_cast<size_t>(count);

  ssize_t nWritten = ::pwrite(_fd, buf.data(), countArg, offArg);

  if (nWritten < 0) {
    return _lastError = Platform::TranslateError();
  }

  count = static_cast<uint64_t>(nWritten);

  return _lastError = kSuccess;
}

ErrorCode File::chmod(std::string const &path, uint32_t mode) {
  if (::chmod(path.c_str(), mode) < 0) {
    return Platform::TranslateError();
  }

  return kSuccess;
}

ErrorCode File::unlink(std::string const &path) {
  if (::unlink(path.c_str()) < 0) {
    return Platform::TranslateError();
  }

  return kSuccess;
}

ErrorCode File::createDirectory(std::string const &path, uint32_t flags) {
  size_t pos = 0;

  // Each parent directory must be created individually, if it does not exist
  do {
    pos = path.find('/', pos + 1);
    std::string partialPath = path.substr(0, pos);

    if (::mkdir(partialPath.c_str(), flags) < 0) {
      ErrorCode error = Platform::TranslateError();
      if (error == kErrorAlreadyExist) {
        continue;
      }

      return error;
    }
  } while (pos != std::string::npos);

  return kSuccess;
}
} // namespace Host
} // namespace ds2
