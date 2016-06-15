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
}
}
