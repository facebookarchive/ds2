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

#include "DebugServer2/Host/Darwin/Mach.h"
#include "DebugServer2/Target/POSIX/Process.h"

namespace ds2 {
namespace Target {
namespace Darwin {

class MachOProcess : public POSIX::Process {
protected:
  std::string _auxiliaryVector;
  Address _sharedLibraryInfoAddress;
  Host::Darwin::Mach _mach;

public:
  ErrorCode getAuxiliaryVector(std::string &auxv) override;
  uint64_t getAuxiliaryVectorValue(uint64_t type) override;

public:
  virtual ErrorCode getSharedLibraryInfoAddress(Address &address);
  ErrorCode enumerateSharedLibraries(
      std::function<void(SharedLibraryInfo const &)> const &cb) override;

public:
  Host::Darwin::Mach &mach();

protected:
  ErrorCode updateInfo() override;
  virtual ErrorCode updateAuxiliaryVector();
};
} // namespace Darwin
} // namespace Target
} // namespace ds2
