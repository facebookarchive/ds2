//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Host/Windows/ExtraWrappers.h"
#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Types.h"
#include "DebugServer2/Utils/Log.h"
#include "DebugServer2/Utils/ScopedJanitor.h"
#include "DebugServer2/Utils/String.h"
#include "DebugServer2/Utils/Stringify.h"

#include <psapi.h>
#include <vector>

using ds2::Host::Platform;
using ds2::Host::ProcessSpawner;
using ds2::Utils::Stringify;

#define super ds2::Target::ProcessBase

#if defined(ARCH_ARM)
static uint8_t const gDebugBreakCode[] = {
    0xFE, 0xDE,             // 00: UDF #254
    0xDF, 0xF8, 0x04, 0x40, // 02: LDR.W R4,[PC, #+0x4]
    0x20, 0x47,             // 06: BX R4
    0x00, 0x00, 0x00, 0x00  // 08: RtlExitUserThread address
};
static const int gRtlExitUserThreadOffset = 0x08;
#elif defined(ARCH_X86)
static uint8_t const gDebugBreakCode[] = {
    0xCC,                         // 00: int 3
    0xB8, 0x00, 0x00, 0x00, 0x00, // 01: mov eax, RtlExitUserThread address
    0xFF, 0xD0                    // 06: call eax
};
static const int gRtlExitUserThreadOffset = 0x02;
#elif defined(ARCH_X86_64)
static uint8_t const gDebugBreakCode[] = {
    0xCC,                         // 00: int 3
    0x48, 0xB8, 0x00, 0x00, 0x00, // 01: movabs rax, RtlExitUserThread address
    0x00, 0x00, 0x00, 0x00, 0x00, // cont.
    0xFF, 0xD0                    // 0b: call eax
};
static const int gRtlExitUserThreadOffset = 0x03;
#endif

namespace ds2 {
namespace Target {
namespace Windows {

Process::Process() : super(), _handle(nullptr) {}

Process::~Process() {
  // On winphone we are unable to use DebugSetProcessKillOnExit to detach on
  // exit, so we detach at this point. This is required because otherwise the
  // debugged winphone process might stay alive in an unclosable state. If
  // detached, the winphone process dies gracefully.
  detach();
  ::CloseHandle(_handle);
}

ErrorCode Process::initialize(ProcessId pid, uint32_t flags) {
  // The first call to `wait()` will receive a CREATE_PROCESS_DEBUG_EVENT event
  // which will fill in `_handle` and create the main thread for this process.
  CHK(wait());

  // Can't use `CHK()` here because of a bug in GCC. See
  // http://stackoverflow.com/a/12086873/5731788
  ErrorCode error = super::initialize(pid, flags);
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
    CHK(resume());
    CHK(wait());
  } while (currentThread()->stopInfo().event != StopInfo::kEventStop ||
           currentThread()->stopInfo().reason != StopInfo::kReasonBreakpoint);

  return kSuccess;
}

Target::Process *Process::Attach(ProcessId pid) {
  if (pid <= 0) {
    return nullptr;
  }

  BOOL result = DebugActiveProcess(pid);
  if (!result) {
    return nullptr;
  }

  DS2LOG(Debug, "attached to process %" PRIu64, (uint64_t)pid);

  auto process = make_protected_unique();

  if (process->initialize(pid, kFlagAttachedProcess) != kSuccess) {
    return nullptr;
  }

  return process.release();
}

ErrorCode Process::detach() {
  prepareForDetach();

  BOOL result = DebugActiveProcessStop(_pid);
  if (!result) {
    return Platform::TranslateError();
  }

  cleanup();
  _flags &= ~kFlagAttachedProcess;

  return kSuccess;
}

ErrorCode Process::writeDebugBreakCode(uint64_t address) {
  FARPROC exitThreadAddress =
      GetProcAddress(GetModuleHandleA("ntdll"), "RtlExitUserThread");
  if (exitThreadAddress == NULL) {
    return Platform::TranslateError();
  }

  ByteVector codestr(&gDebugBreakCode[0],
                     &gDebugBreakCode[sizeof(gDebugBreakCode)]);
  *reinterpret_cast<uintptr_t *>(&codestr[gRtlExitUserThreadOffset]) =
      reinterpret_cast<uintptr_t>(exitThreadAddress);

  size_t written;
  CHK(writeMemory(Address(address), &codestr[0], codestr.size(), &written));
  return kSuccess;
}

ErrorCode Process::interrupt() {
  uint64_t address;

  CHK(allocateMemory(Platform::GetPageSize(),
                     kProtectionExecute | kProtectionWrite, &address));
  CHK(writeDebugBreakCode(address));

  DWORD threadId;
#if defined(ARCH_ARM)
  // +1 indicates the thread to start in THUMB mode
  auto remoteAddress = address + 1;
#else
  auto remoteAddress = address;
#endif
  if (::CreateRemoteThread(_handle, NULL, 0,
                           (LPTHREAD_START_ROUTINE)remoteAddress, NULL, 0,
                           &threadId) == NULL) {
    return Platform::TranslateError();
  }

  return kSuccess;
}

ErrorCode Process::terminate() {
  BOOL result = ::TerminateProcess(_handle, 0);
  if (!result) {
    return Platform::TranslateError();
  }

  _terminated = true;
  return kSuccess;
}

bool Process::isAlive() const { return !_terminated; }

template <typename ThreadCollectionType, typename ThreadIdType>
static Thread *findThread(ThreadCollectionType const &threads,
                          ThreadIdType tid) {
  auto threadIt = threads.find(tid);
  DS2ASSERT(threadIt != threads.end());
  return threadIt->second;
}

ErrorCode Process::wait() {
  // If _terminated is true, we just called Process::Terminate.
  if (_terminated) {
    DS2ASSERT(_currentThread != nullptr);
    _currentThread->_stopInfo.event = StopInfo::kEventKill;
    return kSuccess;
  }

  for (;;) {
    _currentThread = nullptr;

    DEBUG_EVENT de;
    BOOL result = ::WaitForDebugEventEx(&de, INFINITE);
    if (!result)
      return Platform::TranslateError();

    DS2LOG(Debug, "debug event from inferior, event=%s",
           Stringify::DebugEvent(de.dwDebugEventCode));

    switch (de.dwDebugEventCode) {
    case CREATE_PROCESS_DEBUG_EVENT:
      DS2ASSERT(_handle == nullptr);
      DS2ASSERT(de.u.CreateProcessInfo.hProcess != NULL);
      DS2ASSERT(de.u.CreateProcessInfo.hThread != NULL);
      if (de.u.CreateProcessInfo.hFile != NULL) {
        ::CloseHandle(de.u.CreateProcessInfo.hFile);
      }

      _handle = de.u.CreateProcessInfo.hProcess;
      _currentThread =
          new Thread(this, GetThreadId(de.u.CreateProcessInfo.hThread),
                     de.u.CreateProcessInfo.hThread);
      return kSuccess;

    case EXIT_PROCESS_DEBUG_EVENT: {
      // We should have received a few EXIT_THREAD_DEBUG_EVENT events and there
      // should only be one thread left at this point.
      DS2ASSERT(_threads.size() == 1);
      _currentThread = findThread(_threads, de.dwThreadId);

      _terminated = true;
      _currentThread->_state = Thread::kTerminated;

      DWORD exitCode;
      BOOL result = ::GetExitCodeProcess(_handle, &exitCode);
      if (!result) {
        return Platform::TranslateError();
      }

      _currentThread->_stopInfo.event = StopInfo::kEventExit;
      _currentThread->_stopInfo.status = exitCode;
      return kSuccess;
    }

    case CREATE_THREAD_DEBUG_EVENT: {
      _currentThread =
          new Thread(this, de.dwThreadId, de.u.CreateThread.hThread);
      CHK(_currentThread->resume());
      continue;
    }

    case EXIT_THREAD_DEBUG_EVENT: {
      _currentThread = findThread(_threads, de.dwThreadId);
      _currentThread->updateState(de);
      CHK(_currentThread->resume());
      removeThread(_currentThread->tid());
      continue;
    }

    case EXCEPTION_DEBUG_EVENT:
    case LOAD_DLL_DEBUG_EVENT:
    case UNLOAD_DLL_DEBUG_EVENT:
    case OUTPUT_DEBUG_STRING_EVENT: {
      _currentThread = findThread(_threads, de.dwThreadId);
      _currentThread->updateState(de);
      CHK(suspend());
      return kSuccess;
    }

    default:
      DS2BUG("unknown debug event code: %s",
             Stringify::DebugEvent(de.dwDebugEventCode));
    }
  }
}

ErrorCode Process::readString(Address const &address, std::string &str,
                              size_t length, size_t *nread) {
  for (size_t i = 0; i < length; ++i) {
    char c;
    CHK(readMemory(address + i, &c, sizeof(c), nullptr));
    if (c == '\0') {
      return kSuccess;
    }
    str.push_back(c);
  }

  return kSuccess;
}

ErrorCode Process::readMemory(Address const &address, void *data, size_t length,
                              size_t *nread) {
  SIZE_T bytesRead;
  BOOL result =
      ReadProcessMemory(_handle, reinterpret_cast<LPCVOID>(address.value()),
                        data, length, &bytesRead);

  if (nread != nullptr) {
    *nread = static_cast<size_t>(bytesRead);
  }

  if (!result) {
    auto error = GetLastError();
    if (error != ERROR_PARTIAL_COPY || bytesRead == 0) {
      return Host::Platform::TranslateError(error);
    }
  }

  return kSuccess;
}

ErrorCode Process::restoreRegionsProtection(
    const std::vector<MemoryRegionInfo> &regions) {
  for (const auto &region : regions) {
    LPVOID allocError = VirtualAllocEx(
        _handle, reinterpret_cast<LPVOID>(region.start.value()), region.length,
        MEM_COMMIT, convertMemoryProtectionToWindows(region.protection));
    if (allocError == NULL) {
      return Platform::TranslateError();
    }
  }
  return kSuccess;
}

DWORD Process::convertMemoryProtectionToWindows(uint32_t protection) const {
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
  return allocProtection;
}

uint32_t
Process::convertMemoryProtectionFromWindows(DWORD winProtection) const {
  switch (winProtection) {
  case PAGE_EXECUTE_READWRITE:
  case PAGE_EXECUTE_WRITECOPY:
    return kProtectionWrite | kProtectionExecute;
  case PAGE_EXECUTE_READ:
    return kProtectionRead | kProtectionExecute;
  case PAGE_EXECUTE:
    return kProtectionExecute;
  case PAGE_READWRITE:
  case PAGE_WRITECOPY:
    return kProtectionWrite | kProtectionRead;
  case PAGE_READONLY:
    return kProtectionRead;
  // case PAGE_NOACCESS:
  // case PAGE_TARGETS_INVALID:
  // case PAGE_TARGETS_NO_UPDATE:
  default:
    return 0;
  }
}

// TODO: rename this function to getMemoryRegionInfo and make lldb work with it
ErrorCode Process::getMemoryRegionInfoInternal(Address const &address,
                                               MemoryRegionInfo &region) {
  _MEMORY_BASIC_INFORMATION64 mem;
  SIZE_T bytesQuery = VirtualQueryEx(
      _handle, reinterpret_cast<LPVOID>(address.value()),
      reinterpret_cast<PMEMORY_BASIC_INFORMATION>(&mem), sizeof(mem));

  // TODO: Empty string in assignment of region below is a work-around. Fix at
  // some point.
  if (bytesQuery == sizeof(_MEMORY_BASIC_INFORMATION64)) {
    region = {Address(mem.BaseAddress), mem.RegionSize,
              convertMemoryProtectionFromWindows(mem.AllocationProtect), ""};
  } else if (bytesQuery == sizeof(_MEMORY_BASIC_INFORMATION32)) {
    MEMORY_BASIC_INFORMATION32 *mem32 =
        reinterpret_cast<_MEMORY_BASIC_INFORMATION32 *>(&mem);
    region = {Address(mem32->BaseAddress), mem32->RegionSize,
              convertMemoryProtectionFromWindows(mem32->AllocationProtect), ""};
  } else {
    return Platform::TranslateError();
  }
  return kSuccess;
}

ErrorCode
Process::makeMemoryWritable(Address const &address, size_t length,
                            std::vector<MemoryRegionInfo> &modifiedRegions) {
  Address startAddress = address;
  Address endAddress = startAddress + length;

  DS2ASSERT(modifiedRegions.empty());
  auto janitor = Utils::MakeJanitor([this, &modifiedRegions]() {
    this->restoreRegionsProtection(modifiedRegions);
    modifiedRegions.clear();
  });

  while (startAddress < endAddress) {
    MemoryRegionInfo region;
    CHK(getMemoryRegionInfoInternal(address, region));

    if (!region.protection || !(region.protection & kProtectionWrite)) {
      LPVOID allocError = VirtualAllocEx(
          _handle, reinterpret_cast<LPVOID>(region.start.value()),
          region.length, MEM_COMMIT,
          convertMemoryProtectionToWindows(region.protection |
                                           kProtectionWrite));

      // The region could have been modified even if `VirtualAllocEx` failed.
      modifiedRegions.push_back(region);
      if (allocError == NULL) {
        return Platform::TranslateError();
      }
    }
    startAddress = region.start + region.length;
  }

  janitor.disable();
  return kSuccess;
}

ErrorCode Process::writeMemory(Address const &address, void const *data,
                               size_t length, size_t *nwritten) {
  std::vector<MemoryRegionInfo> modifiedRegions;

  CHK(makeMemoryWritable(address, length, modifiedRegions));
  auto janitor = Utils::MakeJanitor([this, &modifiedRegions]() {
    this->restoreRegionsProtection(modifiedRegions);
  });

  SIZE_T bytesWritten;
  BOOL result =
      WriteProcessMemory(_handle, reinterpret_cast<LPVOID>(address.value()),
                         data, length, &bytesWritten);

  if (nwritten != nullptr) {
    *nwritten = static_cast<size_t>(bytesWritten);
  }

  if (!result) {
    auto error = GetLastError();
    if (error != ERROR_PARTIAL_COPY || bytesWritten == 0) {
      return Host::Platform::TranslateError(error);
    }
  }

  if (!FlushInstructionCache(_handle, reinterpret_cast<LPVOID>(address.value()),
                             bytesWritten)) {
    DS2LOG(Warning, "unable to flush instruction cache");
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
  if (spawner.run() != kSuccess) {
    return nullptr;
  }

  DS2LOG(Debug, "created process %" PRIu64, (uint64_t)spawner.pid());

  struct MakeUniqueEnabler : public Process {};
  auto process = ds2::make_unique<MakeUniqueEnabler>();

  if (process->initialize(spawner.pid(), kFlagNewProcess) != kSuccess) {
    return nullptr;
  }

  return process.release();
}

ErrorCode Process::allocateMemory(size_t size, uint32_t protection,
                                  uint64_t *address) {
  DWORD allocProtection = convertMemoryProtectionToWindows(protection);

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
                          static_cast<DWORD>(modules.size() * sizeof(HMODULE)),
                          &bytesNeeded);
  if (!rc)
    return Platform::TranslateError();

  modules.resize(bytesNeeded / sizeof(HMODULE));

  rc = EnumProcessModules(_handle, modules.data(),
                          static_cast<DWORD>(modules.size() * sizeof(HMODULE)),
                          &bytesNeeded);
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
    sl.path = ds2::Utils::WideToNarrowString(std::wstring(nameStr, nameSize));

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
} // namespace Windows
} // namespace Target
} // namespace ds2
