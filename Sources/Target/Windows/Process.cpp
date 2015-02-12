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

#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Target/Windows/Process.h"
#include "DebugServer2/Log.h"

using ds2::Host::ProcessSpawner;

namespace ds2 {
namespace Target {
namespace Windows {

Process::Process() : Target::ProcessBase() {}

Process::~Process() {}

ErrorCode Process::initialize(ProcessId pid, uint32_t flags) {
  // Nothing to do for now.
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
