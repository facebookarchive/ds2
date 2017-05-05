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
