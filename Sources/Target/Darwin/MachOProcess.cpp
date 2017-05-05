//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/Darwin/MachOProcess.h"
#include "DebugServer2/Utils/Log.h"

#include <dirent.h>
#include <limits>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach/thread_info.h>
#include <sys/types.h>

namespace ds2 {
namespace Target {
namespace Darwin {

Host::Darwin::Mach &MachOProcess::mach() { return _mach; }

ErrorCode MachOProcess::getAuxiliaryVector(std::string &auxv) {
  ErrorCode error = updateAuxiliaryVector();
  if (error == kSuccess || error == kErrorAlreadyExist) {
    auxv = _auxiliaryVector;
    error = kSuccess;
  }
  return error;
}

uint64_t MachOProcess::getAuxiliaryVectorValue(uint64_t type) {
  DS2BUG("not implemented");
}

//
// Inheriting classes should override and call this method
// to complete the process information; if this method
// returns kErrorAlreadyExist then the information is
// already present and the call should be considered
// successful.
//
ErrorCode MachOProcess::updateInfo() {
  if (_info.pid == _pid)
    return kErrorAlreadyExist;

  // This is tricky, we don't know the load base, and to
  // do so we need to update the auxiliary vector, but
  // in order to interpret it we need to determine first
  // if it's 32 or 64bits. In this point of the code
  // we don't have idea of what is our target platform,
  // so we'll do an empirical test.
  if (!_loadBase.valid() || !_entryPoint.valid()) {
    ErrorCode error = updateAuxiliaryVector();
    if (error != kSuccess && error != kErrorAlreadyExist) {
      return error;
    }

    // Hack to use enumerateAuxiliaryVector before information is set.
    _info.pid = _pid;

    bool possibly32 = false;
    for (size_t n = 0; n < _auxiliaryVector.size() / (sizeof(uint64_t) * 2);
         n += 2) {
      if (*(uint64_t *)&_auxiliaryVector[n * sizeof(uint64_t) * 2] >
          std::numeric_limits<uint32_t>::max()) {
        possibly32 = true;
      }
    }

    if (!possibly32) {
      _info.cpuType = kCPUTypeAll64;
    } else {
      _info.cpuType = kCPUTypeAll;
    }
  }

  _entryPoint = 0; // TODO
  _loadBase = 0;   // TODO

  _info.osType = "macosx";
  _info.osVendor = "apple";

  _info.cpuType = kCPUTypeX86_64;           // TODO
  _info.cpuSubType = kCPUSubTypeX86_64_ALL; // TODO
  _info.nativeCPUType = 0;                  // TODO
  _info.nativeCPUSubType = kInvalidCPUType; // TODO
  _info.pid = _pid;
  _info.endian = kEndianLittle; // TODO kEndianBig : kEndianLittle;
  _info.pointerSize = 8;        // TODO

  _info.realGid = 0;      // TODO
  _info.effectiveGid = 0; // TODO;
  _info.realUid = 0;      // TODO
  _info.effectiveUid = 0; // TODO

  return kSuccess;
}

//
// Inheriting class should call this method and then
// read data into _auxiliaryVector buffer; if this method
// returns kErrorAlreadyExist then the information is
// already present and the call should be considered
// successful, any other error should be ignored.
//
ErrorCode MachOProcess::updateAuxiliaryVector() { return kSuccess; }

//
// Retrieves the shared library info address pointer.
//
ErrorCode MachOProcess::getSharedLibraryInfoAddress(Address &address) {
  return mach().getProcessDylbInfo(_info.pid, address);
}

//
// Enumerates the linkmap of this MachO process.
//
ErrorCode MachOProcess::enumerateSharedLibraries(
    std::function<void(SharedLibraryInfo const &)> const &cb) {
  Address address;
  CHK(getSharedLibraryInfoAddress(address));
  DS2BUG("not implemented");
}
} // namespace Darwin
} // namespace Target
} // namespace ds2
