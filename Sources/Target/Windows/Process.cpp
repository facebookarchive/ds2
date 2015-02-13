//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#define __DS2_LOG_CLASS_NAME__ "Target::Process"

#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Target/Windows/Process.h"
#include "DebugServer2/Utils/Log.h"

using ds2::Host::ProcessSpawner;
using ds2::Host::Platform;

#define super ds2::Target::ProcessBase

namespace ds2 {
namespace Target {
namespace Windows {

Process::Process() : Target::ProcessBase() {}

Process::~Process() {}

ErrorCode Process::initialize(ProcessId pid, uint32_t flags) {
  // Nothing to do for now.
  return super::initialize(pid, flags);
}

ErrorCode Process::updateInfo() {
  if (_info.pid == _pid)
    return kErrorAlreadyExist;

  _info.pid = _pid;

  // Note(sas): We can't really return UID/GID at the moment. Windows doesn't
  // have simple integer IDs.
  _info.realUid = 0;
  _info.realGid = 0;

  _info.cpuType = Platform::GetCPUType();
  _info.cpuSubType = Platform::GetCPUSubType();

  // FIXME(sas): nativeCPU{,sub}Type are the values that the debugger
  // understands and that we will send on the wire. For ELF processes, it will
  // be the values gotten from the ELF header. Not sure what it is for PE
  // processes yet.
  _info.nativeCPUType = _info.cpuType;
  _info.nativeCPUSubType = _info.cpuSubType;

  // No big endian on Windows.
  _info.endian = kEndianLittle;

  _info.pointerSize = Platform::GetPointerSize();

  // FIXME(sas): No idea what this field is. It looks completely unused in the
  // rest of the source.
  _info.archFlags = 0;

  _info.osType = Platform::GetOSTypeName();
  _info.osVendor = Platform::GetOSVendorName();

  return kSuccess;
}

ds2::Target::Process *Process::Create(ProcessSpawner &spawner) {
  ErrorCode error;
  ProcessId pid;

  //
  // Create the process.
  //
  Target::Process *process = new Target::Process;

  error = spawner.run();
  if (error != kSuccess)
    goto fail;

  pid = spawner.pid();

  //
  // Wait the process.
  //
  error = process->initialize(pid, 0);
  if (error != kSuccess)
    goto fail;

  return process;

fail:
  delete process;
  return nullptr;
}
}
}
}
