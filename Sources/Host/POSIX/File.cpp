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
#include <unistd.h>

namespace ds2 {
namespace Host {

File::File(std::string const &path, uint32_t flags, uint32_t mode) {
  _fd = ::open(path.c_str(), flags, mode);
  _lastError = (_fd < 0) ? Platform::TranslateError() : kSuccess;
}

File::~File() {
  if (valid()) {
    ::close(_fd);
  }
}

ErrorCode File::pread(std::string &buf, uint64_t &count, uint64_t offset) {
  if (!valid()) {
    return _lastError = kErrorInvalidHandle;
  }

  if (offset > std::numeric_limits<off_t>::max()) {
    return _lastError = kErrorInvalidArgument;
  }

  auto offArg = static_cast<off_t>(offset);
  auto countArg = static_cast<size_t>(count);

  buf.resize(countArg);

  ssize_t nRead = ::pread(_fd, &buf[0], countArg, offArg);

  if (nRead < 0) {
    return _lastError = Platform::TranslateError();
  }

  buf.resize(nRead);
  count = static_cast<uint64_t>(nRead);

  return _lastError = kSuccess;
}
}
}
