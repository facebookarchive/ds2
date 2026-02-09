// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#include "DebugServer2/Host/File.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Host/Windows/ExtraWrappers.h"

namespace ds2 {
namespace Host {

File::File(std::string const &path, OpenFlags flags, uint32_t mode)
    : _fd(-1), _lastError(kErrorUnsupported) {}

File::~File() = default;

ErrorCode File::pread(ByteVector &buf, uint64_t &count, uint64_t offset) {
  return kErrorUnsupported;
}

ErrorCode File::pwrite(ByteVector const &buf, uint64_t &count,
                       uint64_t offset) {
  return kErrorUnsupported;
}

ErrorCode File::chmod(std::string const &path, uint32_t mode) {
  return kErrorUnsupported;
}

ErrorCode File::unlink(std::string const &path) { return kErrorUnsupported; }

ErrorCode File::createDirectory(std::string const &path, uint32_t flags) {
  return kErrorUnsupported;
}
} // namespace Host
} // namespace ds2
