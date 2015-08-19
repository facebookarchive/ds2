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

#include <psapi.h>

using ds2::Host::ProcessSpawner;
using ds2::Host::Platform;

#define super ds2::Target::ProcessBase

namespace ds2 {
namespace Target {
namespace Windows {

Process::Process()
    : Target::ProcessBase(), _handle(nullptr), _breakpointManager(nullptr),
      _terminated(false) {}

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

  switch (de.dwDebugEventCode) {
  case CREATE_PROCESS_DEBUG_EVENT:
#define CHECK_AND_CLOSE(HAN)                                                   \
  do {                                                                         \
    if ((de.u.CreateProcessInfo.HAN) != NULL)                                  \
      CloseHandle(de.u.CreateProcessInfo.HAN);                                 \
  } while (0)
    CHECK_AND_CLOSE(hFile);
    CHECK_AND_CLOSE(hProcess);
    CHECK_AND_CLOSE(hThread);
#undef CHECK_AND_CLOSE
    return kSuccess;

  case EXIT_PROCESS_DEBUG_EVENT:
    _terminated = true;
    return kSuccess;

  case CREATE_THREAD_DEBUG_EVENT:
    DS2LOG(Fatal, "debug event CREATE_THREAD");
  case EXIT_THREAD_DEBUG_EVENT:
    DS2LOG(Fatal, "debug event EXIT_THREAD");
  case RIP_EVENT:
    DS2LOG(Fatal, "debug event RIP");

  case EXCEPTION_DEBUG_EVENT:
  case LOAD_DLL_DEBUG_EVENT:
  case UNLOAD_DLL_DEBUG_EVENT:
  case OUTPUT_DEBUG_STRING_EVENT: {
    auto threadIt = _threads.find(de.dwThreadId);
    DS2ASSERT(threadIt != _threads.end());
    threadIt->second->updateState(de);
  } break;

  default:
    DS2BUG("unknown debug event code: %d", de.dwDebugEventCode);
  }

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
      DS2LOG(Debug, "resuming tid %d from pc %#llx", thread->tid(),
             (unsigned long long)state.pc());
      ErrorCode error = thread->resume(signal);
      if (error != kSuccess) {
        DS2LOG(Warning, "failed resuming tid %d, error=%d", thread->tid(),
               error);
      }
    }
  });

  return kSuccess;
}

ErrorCode Process::readMemory(Address const &address, void *data, size_t length,
                              size_t *nread) {
  BOOL result =
      ReadProcessMemory(_handle, reinterpret_cast<LPCVOID>(address.value()),
                        data, length, reinterpret_cast<SIZE_T *>(nread));

  if (!result) {
    auto error = GetLastError();
    if (error != ERROR_PARTIAL_COPY)
      return Host::Platform::TranslateError(error);
  }

  return kSuccess;
}

ErrorCode Process::writeMemory(Address const &address, void const *data,
                               size_t length, size_t *nwritten) {
  BOOL result =
      WriteProcessMemory(_handle, reinterpret_cast<LPVOID>(address.value()),
                         data, length, reinterpret_cast<SIZE_T *>(nwritten));

  if (!result) {
    auto error = GetLastError();
    if (error != ERROR_PARTIAL_COPY)
      return Host::Platform::TranslateError(error);
  }

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

  DS2LOG(Debug, "created process %d", spawner.pid());

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

ErrorCode Process::allocateMemory(size_t size, uint32_t protection,
                                  uint64_t *address) {
  DWORD allocProtection = 0;

  if (protection & kProtectionExecute) {
    if (protection & kProtectionWrite)
      allocProtection = PAGE_EXECUTE_READWRITE;
    else if (protection & kProtectionRead)
      allocProtection = PAGE_EXECUTE_READ;
    else
      allocProtection = PAGE_EXECUTE;
  } else {
    if (protection & kProtectionWrite)
      allocProtection = PAGE_READWRITE;
    else if (protection & kProtectionRead)
      allocProtection = PAGE_READONLY;
    else
      allocProtection = PAGE_NOACCESS;
  }

  LPVOID result = VirtualAllocEx(_handle, nullptr, size,
                                 MEM_COMMIT | MEM_RESERVE, allocProtection);

  if (result == NULL)
    return Platform::TranslateError();

  *address = reinterpret_cast<uint64_t>(result);
  return kSuccess;
}

ErrorCode Process::deallocateMemory(uint64_t address, size_t size) {
  BOOL result =
      VirtualFreeEx(_handle, reinterpret_cast<LPVOID>(address), 0, MEM_RELEASE);

  if (!result)
    return Platform::TranslateError();

  return kSuccess;
}

ErrorCode Process::enumerateSharedLibraries(
    std::function<void(SharedLibraryInfo const &)> const &cb) {
  BOOL rc;
  std::vector<HMODULE> modules;
  DWORD bytesNeeded;

  rc = EnumProcessModules(_handle, modules.data(),
                          modules.size() * sizeof(HMODULE), &bytesNeeded);
  if (!rc)
    return Platform::TranslateError();

  modules.resize(bytesNeeded / sizeof(HMODULE));

  rc = EnumProcessModules(_handle, modules.data(),
                          modules.size() * sizeof(HMODULE), &bytesNeeded);
  if (!rc)
    return Platform::TranslateError();

  for (auto m : modules) {
    SharedLibraryInfo sl;

    sl.main = (m == modules[0]);

    WCHAR nameStr[MAX_PATH];
    DWORD nameSize;
    nameSize = GetModuleFileNameExW(_handle, m, nameStr, sizeof(nameStr));
    if (nameSize == 0)
      return Platform::TranslateError();
    sl.path = Platform::WideToNarrowString(std::wstring(nameStr, nameSize));

    // Modules on Windows only have one "section", which is the address of the
    // module itself.
    sl.sections.push_back(reinterpret_cast<uint64_t>(m));

    cb(sl);
  }

  return kSuccess;
}
}
}
}
