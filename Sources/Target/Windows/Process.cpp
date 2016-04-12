//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#define __DS2_LOG_CLASS_NAME__ "Target::Process"

#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Host/Windows/ExtraWrappers.h"
#include "DebugServer2/Support/Stringify.h"
#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Utils/Log.h"

#include <psapi.h>
#include <vector>

using ds2::Host::ProcessSpawner;
using ds2::Host::Platform;
using ds2::Support::Stringify;

#define super ds2::Target::ProcessBase

namespace ds2 {
namespace Target {
namespace Windows {

Process::Process()
    : Target::ProcessBase(), _handle(nullptr),
      _softwareBreakpointManager(nullptr), _terminated(false) {}

Process::~Process() { CloseHandle(_handle); }

ErrorCode Process::initialize(ProcessId pid, uint32_t flags) {
  // The first call to `wait()` will receive a CREATE_PROCESS_DEBUG_EVENT event
  // which will fill in `_handle` and create the main thread for this process.
  ErrorCode error = wait();
  if (error != kSuccess) {
    return error;
  }

  error = super::initialize(pid, flags);
  if (error != kSuccess) {
    return error;
  }

  // Then we can continue resuming and waiting for the process until we hit a
  // breakpoint.
  // If we are creating the process ourselves, the first breakpoint will be in
  // some system library before running user code.
  // If we are attaching to an already running process, we will break in
  // `DbgBreakPoint`, which is called by the remote thread `DebugActiveProcess`
  // creates.
  do {
    error = resume();
    if (error != kSuccess) {
      return error;
    }
    error = wait();
    if (error != kSuccess) {
      return error;
    }
  } while (currentThread()->stopInfo().event != StopInfo::kEventStop ||
           currentThread()->stopInfo().reason != StopInfo::kReasonBreakpoint);

  return kSuccess;
}

Target::Process *Process::Attach(ProcessId pid) {
  if (pid <= 0)
    return nullptr;

  BOOL result = DebugActiveProcess(pid);
  if (!result) {
    return nullptr;
  }

  DS2LOG(Debug, "attached to process %" PRIu64, (uint64_t)pid);

  auto process = new Process;
  ErrorCode error = process->initialize(pid, 0);
  if (error != kSuccess) {
    delete process;
    return nullptr;
  }

  return process;
}

ErrorCode Process::detach() {
  prepareForDetach();

  BOOL result = DebugActiveProcessStop(_pid);
  if (!result)
    return Platform::TranslateError();

  cleanup();
  _flags &= ~kFlagAttachedProcess;

  return kSuccess;
}

ErrorCode Process::interrupt() {
  BOOL result = DebugBreakProcess(_handle);
  if (!result)
    return Platform::TranslateError();

  return kSuccess;
}

ErrorCode Process::terminate() {
  BOOL result = TerminateProcess(_handle, 0);
  if (!result)
    return Platform::TranslateError();

  _terminated = true;
  return detach();
}

bool Process::isAlive() const { return !_terminated; }

ErrorCode Process::wait() {
  if (_terminated)
    return kSuccess;

  for (;;) {
    _currentThread = nullptr;

    DEBUG_EVENT de;
    BOOL result = WaitForDebugEvent(&de, INFINITE);
    if (!result)
      return Platform::TranslateError();

    DS2LOG(Debug, "debug event from inferior, event=%s",
           Stringify::DebugEvent(de.dwDebugEventCode));

    switch (de.dwDebugEventCode) {
    case CREATE_PROCESS_DEBUG_EVENT:
      DS2ASSERT(_handle == nullptr);
      DS2ASSERT(de.u.CreateProcessInfo.hProcess != NULL);
      _handle = de.u.CreateProcessInfo.hProcess;
      _currentThread =
          new Thread(this, GetThreadId(de.u.CreateProcessInfo.hThread),
                     de.u.CreateProcessInfo.hThread);
      if (de.u.CreateProcessInfo.hFile != NULL) {
        CloseHandle(de.u.CreateProcessInfo.hFile);
      }
      return kSuccess;

    case EXIT_PROCESS_DEBUG_EVENT:
      _terminated = true;
      return kSuccess;

    case CREATE_THREAD_DEBUG_EVENT: {
      _currentThread = new Thread(this, GetThreadId(de.u.CreateThread.hThread),
                                  de.u.CreateThread.hThread);
      ErrorCode error = _currentThread->resume();
      if (error != kSuccess) {
        return error;
      }
      continue;
    }

    case EXIT_THREAD_DEBUG_EVENT: {
      auto threadIt = _threads.find(de.dwThreadId);
      DS2ASSERT(threadIt != _threads.end());

      _currentThread = threadIt->second;
      ErrorCode error = _currentThread->resume();
      if (error != kSuccess) {
        return error;
      }
      removeThread(_currentThread->tid());
      continue;
    }

    case RIP_EVENT:
      DS2LOG(Fatal, "debug event RIP");

    case EXCEPTION_DEBUG_EVENT:
    case LOAD_DLL_DEBUG_EVENT:
    case UNLOAD_DLL_DEBUG_EVENT:
    case OUTPUT_DEBUG_STRING_EVENT: {
      // When WaitForDebugEvent returns successfully, all threads are stopped.
      for (auto const &e : _threads) {
        auto t = e.second;
        t->_state = Thread::kStopped;
        t->_stopInfo.event = StopInfo::kEventStop;
        t->_stopInfo.reason = StopInfo::kReasonNone;
      }

      auto threadIt = _threads.find(de.dwThreadId);
      DS2ASSERT(threadIt != _threads.end());
      _currentThread = threadIt->second;
      _currentThread->updateState(de);
      return kSuccess;
    }

    default:
      DS2BUG("unknown debug event code: %lu", de.dwDebugEventCode);
    }
  }
}

ErrorCode Process::resume(int signal, std::set<Thread *> const &excluded) {
  enumerateThreads([&](Thread *thread) {
    if (excluded.find(thread) != excluded.end())
      return;

    if (thread->state() == Thread::kStopped ||
        thread->state() == Thread::kStepped) {
      Architecture::CPUState state;
      thread->readCPUState(state);
      DS2LOG(Debug, "resuming tid %" PRIu64 " from pc %#" PRIx64,
             (uint64_t)thread->tid(), (uint64_t)state.pc());
      ErrorCode error = thread->resume(signal);
      if (error != kSuccess) {
        DS2LOG(Warning, "failed resuming tid %" PRIu64 ", error=%d",
               (uint64_t)thread->tid(), error);
      }
    }
  });

  return kSuccess;
}

ErrorCode Process::readString(Address const &address, std::string &str,
                              size_t length, size_t *nread) {
  return kErrorUnsupported;
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
  ErrorCode error = spawner.run();
  if (error != kSuccess) {
    return nullptr;
  }

  DS2LOG(Debug, "created process %" PRIu64, (uint64_t)spawner.pid());

  auto process = new Process;
  error = process->initialize(spawner.pid(), 0);
  if (error != kSuccess) {
    delete process;
    return nullptr;
  }

  return process;
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

    // The following two transforms ensure that the paths we return to the
    // debugger look like unix paths. This shouldn't be required but LLDB seems
    // to be having trouble with paths when the host and the remote don't use
    // the same path separator.
    if (sl.path.length() >= 2 && sl.path[0] >= 'A' && sl.path[0] <= 'Z' &&
        sl.path[1] == ':')
      sl.path.erase(0, 2);
    for (auto &c : sl.path)
      if (c == '\\')
        c = '/';

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
