// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#pragma once

#include "DebugServer2/Support/POSIX/ELFSupport.h"
#include "DebugServer2/Target/POSIX/Process.h"

namespace ds2 {
namespace Target {
namespace POSIX {

class ELFProcess : public POSIX::Process {
protected:
  std::string _auxiliaryVector;
  Address _sharedLibraryInfoAddress;

public:
  ErrorCode getAuxiliaryVector(std::string &auxv) override;
  uint64_t getAuxiliaryVectorValue(uint64_t type) override;

public:
  virtual ErrorCode getSharedLibraryInfoAddress(Address &address);
  ErrorCode enumerateSharedLibraries(
      std::function<void(SharedLibraryInfo const &)> const &cb) override;

public:
  virtual ErrorCode enumerateAuxiliaryVector(
      std::function<
          void(Support::ELFSupport::AuxiliaryVectorEntry const &)> const &cb);

protected:
  ErrorCode updateInfo() override;
  virtual ErrorCode updateAuxiliaryVector();
};
} // namespace POSIX
} // namespace Target
} // namespace ds2
