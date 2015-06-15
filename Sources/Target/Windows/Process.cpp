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
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Utils/Log.h"

using ds2::Host::ProcessSpawner;
using ds2::Host::Platform;

#define super ds2::Target::ProcessBase

namespace ds2 {
namespace Target {
namespace Windows {

Process::Process()
    : Target::ProcessBase(), _handle(nullptr), _terminated(false) {}

Process::~Process() { CloseHandle(_handle); }

ErrorCode Process::initialize(ProcessId pid, HANDLE handle, ThreadId tid,
                              HANDLE threadHandle, uint32_t flags) {
  int status;
  ErrorCode error = wait(&status, true);
  if (error != kSuccess)
    return error;

  error = super::initialize(pid, flags);
  if (error != kSuccess)
    return error;

  _handle = handle;

  _currentThread = new Thread(this, tid, threadHandle);
  _currentThread->updateTrapInfo();

  return kSuccess;
}

ErrorCode Process::attach(bool reattach) { return kErrorUnsupported; }

ErrorCode Process::detach() { return kErrorUnsupported; }

ErrorCode Process::terminate() {
  BOOL result = TerminateProcess(_handle, 0);
  if (!result)
    return Platform::TranslateError();

  _terminated = true;
  return kSuccess;
}

bool Process::isAlive() const { return !_terminated; }

ErrorCode Process::wait(int *status, bool hang) {
  DEBUG_EVENT de;

  BOOL result = WaitForDebugEvent(&de, hang ? INFINITE : 0);
  if (!result)
    return Platform::TranslateError();

  auto threadIt = _threads.find(de.dwThreadId);

  return kSuccess;
}

ErrorCode Process::resume(int signal, std::set<Thread *> const &excluded) {
  enumerateThreads([&](Thread *thread) {
    if (excluded.find(thread) != excluded.end())
      return;

    if (thread->state() == Thread::kStopped ||
        thread->state() == Thread::kStepped) {
      Architecture::CPUState state;
      thread->readCPUState(state);
      DS2LOG(Target, Debug, "resuming tid %d from pc %#llx", thread->tid(),
             (unsigned long long)state.pc());
      ErrorCode error = thread->resume(signal);
      if (error != kSuccess) {
        DS2LOG(Target, Warning, "failed resuming tid %d, error=%d",
               thread->tid(), error);
      }
    }
  });

  return kSuccess;
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

  //
  // Create the process.
  //
  Target::Process *process = new Target::Process;

  error = spawner.run();
  if (error != kSuccess)
    goto fail;

  DS2LOG(Target, Debug, "created process %d", spawner.pid());

  //
  // Wait the process.
  //
  error = process->initialize(spawner.pid(), spawner.handle(), spawner.tid(),
                              spawner.threadHandle(), 0);
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
