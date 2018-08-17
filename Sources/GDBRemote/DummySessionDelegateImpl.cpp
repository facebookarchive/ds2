//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/GDBRemote/DummySessionDelegateImpl.h"
#include "DebugServer2/GDBRemote/Session.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"

using ds2::Host::Platform;

namespace ds2 {
namespace GDBRemote {

#define DUMMY_IMPL_EMPTY(NAME, ...)                                            \
  ErrorCode DummySessionDelegateImpl::NAME(__VA_ARGS__) {                      \
    return kErrorUnsupported;                                                  \
  }

#define DUMMY_IMPL_EMPTY_CONST(NAME, ...)                                      \
  ErrorCode DummySessionDelegateImpl::NAME(__VA_ARGS__) const {                \
    return kErrorUnsupported;                                                  \
  }

DummySessionDelegateImpl::DummySessionDelegateImpl() = default;

size_t DummySessionDelegateImpl::getGPRSize() const {
  DS2ASSERT(!"this method shall be implemented by inheriting class");
  return 0;
}

ErrorCode DummySessionDelegateImpl::onEnableExtendedMode(Session &) {
  return kSuccess;
}

ErrorCode DummySessionDelegateImpl::onSetBaudRate(Session &, uint32_t) {
  return kSuccess;
}

ErrorCode DummySessionDelegateImpl::onToggleDebugFlag(Session &) {
  return kSuccess;
}

DUMMY_IMPL_EMPTY(onSetMaxPacketSize, Session &, size_t)

DUMMY_IMPL_EMPTY(onSetMaxPayloadSize, Session &, size_t)

DUMMY_IMPL_EMPTY(onSetLogging, Session &, std::string const &,
                 std::string const &, StringCollection const &)

DUMMY_IMPL_EMPTY(onSendInput, Session &, ByteVector const &)

ErrorCode DummySessionDelegateImpl::onAllowOperations(
    Session &, std::map<std::string, bool> const &) {
  return kSuccess;
}

ErrorCode DummySessionDelegateImpl::onQuerySupported(
    Session &, Feature::Collection const &, Feature::Collection &) const {
  return kSuccess;
}

DUMMY_IMPL_EMPTY(onExecuteCommand, Session &, std::string const &command)

ErrorCode
DummySessionDelegateImpl::onQueryServerVersion(Session &,
                                               ServerVersion &version) const {
  // TODO use #defines
  version.name = "DebugServer2";
  version.version = "1.0.0";
  version.releaseName = "turin";
  version.majorVersion = 1;
  version.minorVersion = 0;
  version.buildNumber = 0;

  return kSuccess;
}

ErrorCode DummySessionDelegateImpl::onQueryHostInfo(Session &,
                                                    HostInfo &info) const {
  info.cpuType = Platform::GetCPUType();
  info.cpuSubType = Platform::GetCPUSubType();
  info.osType = Platform::GetOSTypeName();
  info.osVendor = Platform::GetOSVendorName();

  char const *version = Platform::GetOSVersion();
  if (version != nullptr) {
    info.osVersion = version;
  }

  char const *build = Platform::GetOSBuild();
  if (build != nullptr) {
    info.osBuild = build;
  }

  char const *kernel = Platform::GetOSKernelPath();
  if (kernel != nullptr) {
    info.osKernel = kernel;
  }

  info.endian = Platform::GetEndian();
  info.pointerSize = Platform::GetPointerSize();
  info.hostName = Platform::GetHostName(/*fqdn=*/true);

  // In complex workflows (such as used when testing Android-ARM on CircleCI) we
  // can occasionally experience long delays that exceed the 1 second default
  // timeout. Setting this very high causes no known problems while fixing test
  // failures.
  info.defaultPacketTimeout = 15;

  return kSuccess;
}

DUMMY_IMPL_EMPTY(onQueryFileLoadAddress, Session &, std::string const &,
                 Address &)

DUMMY_IMPL_EMPTY(onEnableControlAgent, Session &, bool)

DUMMY_IMPL_EMPTY(onNonStopMode, Session &, bool)

DUMMY_IMPL_EMPTY(onEnableBTSTracing, Session &, bool)

DUMMY_IMPL_EMPTY(onPassSignals, Session &, std::vector<int> const &)

DUMMY_IMPL_EMPTY(onProgramSignals, Session &, std::vector<int> const &)

DUMMY_IMPL_EMPTY_CONST(onQuerySymbol, Session &, std::string const &,
                       std::string const &, std::string &)

DUMMY_IMPL_EMPTY_CONST(onQueryRegisterInfo, Session &, uint32_t, RegisterInfo &)

DUMMY_IMPL_EMPTY(onAttach, Session &, ProcessId, AttachMode, StopInfo &)

DUMMY_IMPL_EMPTY(onAttach, Session &, std::string const &, AttachMode,
                 StopInfo &)

DUMMY_IMPL_EMPTY(onRunAttach, Session &, std::string const &,
                 StringCollection const &, StopInfo &)

DUMMY_IMPL_EMPTY(onDetach, Session &, ProcessId, bool)

DUMMY_IMPL_EMPTY_CONST(onQueryAttached, Session &, ProcessId, bool &)

DUMMY_IMPL_EMPTY_CONST(onQueryProcessInfo, Session &, ProcessInfo &)

DUMMY_IMPL_EMPTY_CONST(onQueryHardwareWatchpointCount, Session &, size_t &)

DUMMY_IMPL_EMPTY_CONST(onQuerySectionOffsets, Session &, Address &, Address &,
                       bool &)

DUMMY_IMPL_EMPTY_CONST(onQuerySharedLibrariesInfoAddress, Session &, Address &)

DUMMY_IMPL_EMPTY_CONST(onQuerySharedLibraryInfo, Session &,
                       std::string const &path, std::string const &triple,
                       SharedLibraryInfo &info)

DUMMY_IMPL_EMPTY(onRestart, Session &, ProcessId)

DUMMY_IMPL_EMPTY(onInterrupt, Session &)

DUMMY_IMPL_EMPTY(onTerminate, Session &, ProcessThreadId const &, StopInfo &)

DUMMY_IMPL_EMPTY(onExitServer, Session &)

DUMMY_IMPL_EMPTY(onSynchronizeThreadState, Session &, ProcessId)

DUMMY_IMPL_EMPTY_CONST(onQueryThreadList, Session &, ProcessId, ThreadId,
                       ThreadId &)

DUMMY_IMPL_EMPTY_CONST(onQueryCurrentThread, Session &, ProcessThreadId &)

DUMMY_IMPL_EMPTY_CONST(onQueryThreadStopInfo, Session &,
                       ProcessThreadId const &, StopInfo &)

DUMMY_IMPL_EMPTY(onThreadIsAlive, Session &, ProcessThreadId const &)

DUMMY_IMPL_EMPTY_CONST(onQueryThreadInfo, Session &, ProcessThreadId const &,
                       uint32_t, void *)

DUMMY_IMPL_EMPTY_CONST(onQueryTLSAddress, Session &, ProcessThreadId const &,
                       Address const &, Address const &, Address &)

DUMMY_IMPL_EMPTY_CONST(onQueryTIBAddress, Session &, ProcessThreadId const &,
                       Address &)

DUMMY_IMPL_EMPTY(onEnableAsynchronousProfiling, Session &,
                 ProcessThreadId const &, bool, uint32_t, uint32_t)

DUMMY_IMPL_EMPTY_CONST(onQueryProfileData, Session &, ProcessThreadId const &,
                       uint32_t, void *)

DUMMY_IMPL_EMPTY(onResume, Session &, ThreadResumeAction::Collection const &,
                 StopInfo &)

DUMMY_IMPL_EMPTY(onReadGeneralRegisters, Session &, ProcessThreadId const &,
                 Architecture::GPRegisterValueVector &)

DUMMY_IMPL_EMPTY(onWriteGeneralRegisters, Session &, ProcessThreadId const &,
                 std::vector<uint64_t> const &)

DUMMY_IMPL_EMPTY(onReadRegisterValue, Session &, ProcessThreadId const &,
                 uint32_t, std::string &)

DUMMY_IMPL_EMPTY(onWriteRegisterValue, Session &, ProcessThreadId const &,
                 uint32_t, std::string const &)

DUMMY_IMPL_EMPTY(onSaveRegisters, Session &, ProcessThreadId const &ptid,
                 uint64_t &id)

DUMMY_IMPL_EMPTY(onRestoreRegisters, Session &, ProcessThreadId const &ptid,
                 uint64_t id)

DUMMY_IMPL_EMPTY(onReadMemory, Session &, Address const &, size_t, ByteVector &)

DUMMY_IMPL_EMPTY(onWriteMemory, Session &, Address const &, ByteVector const &,
                 size_t &)

DUMMY_IMPL_EMPTY(onAllocateMemory, Session &, size_t, uint32_t, Address &)

DUMMY_IMPL_EMPTY(onDeallocateMemory, Session &, Address const &)

DUMMY_IMPL_EMPTY_CONST(onQueryMemoryRegionInfo, Session &, Address const &,
                       MemoryRegionInfo &)

DUMMY_IMPL_EMPTY(onComputeCRC, Session &, Address const &, size_t, uint32_t &)

DUMMY_IMPL_EMPTY(onSearchBackward, Session &, Address const &, uint32_t,
                 uint32_t, Address &)

DUMMY_IMPL_EMPTY(onSearch, Session &, Address const &, std::string const &,
                 Address &)

DUMMY_IMPL_EMPTY(onInsertBreakpoint, Session &, BreakpointType, Address const &,
                 uint32_t, StringCollection const &, StringCollection const &,
                 bool)

DUMMY_IMPL_EMPTY(onRemoveBreakpoint, Session &, BreakpointType, Address const &,
                 uint32_t)

DUMMY_IMPL_EMPTY(onXferRead, Session &, std::string const &,
                 std::string const &, uint64_t, uint64_t, std::string &, bool &)

DUMMY_IMPL_EMPTY(onXferWrite, Session &, std::string const &,
                 std::string const &, uint64_t, std::string const &, size_t &)

DUMMY_IMPL_EMPTY(onDisableASLR, Session &, bool)

DUMMY_IMPL_EMPTY(onSetEnvironmentVariable, Session &, std::string const &,
                 std::string const &)

DUMMY_IMPL_EMPTY(onSetWorkingDirectory, Session &, std::string const &)

DUMMY_IMPL_EMPTY(onSetStdFile, Session &, int, std::string const &)

DUMMY_IMPL_EMPTY(onSetArchitecture, Session &, std::string const &)

DUMMY_IMPL_EMPTY(onSetProgramArguments, Session &, StringCollection const &)

DUMMY_IMPL_EMPTY(onExecuteProgram, Session &, std::string const &, uint32_t,
                 std::string const &, ProgramResult &)

DUMMY_IMPL_EMPTY(onFileCreateDirectory, Session &, std::string const &path,
                 uint32_t flags)

DUMMY_IMPL_EMPTY(onFileOpen, Session &, std::string const &path,
                 OpenFlags flags, uint32_t mode, int &fd)

DUMMY_IMPL_EMPTY(onFileClose, Session &, int fd)

DUMMY_IMPL_EMPTY(onFileRead, Session &, int fd, uint64_t &count,
                 uint64_t offset, ByteVector &buffer)

DUMMY_IMPL_EMPTY(onFileWrite, Session &, int, uint64_t, ByteVector const &,
                 uint64_t &)

DUMMY_IMPL_EMPTY(onFileRemove, Session &, std::string const &path)

DUMMY_IMPL_EMPTY(onFileReadLink, Session &, std::string const &, std::string &)

DUMMY_IMPL_EMPTY(onFileSetPermissions, Session &, std::string const &, uint32_t)

DUMMY_IMPL_EMPTY(onFileExists, Session &, std::string const &)

DUMMY_IMPL_EMPTY(onFileComputeMD5, Session &, std::string const &, uint8_t[16])

DUMMY_IMPL_EMPTY(onFileGetSize, Session &, std::string const &, uint64_t &)

DUMMY_IMPL_EMPTY_CONST(onQueryProcessList, Session &, ProcessInfoMatch const &,
                       bool, ProcessInfo &)

DUMMY_IMPL_EMPTY_CONST(onQueryProcessInfo, Session &, ProcessId, ProcessInfo &)

DUMMY_IMPL_EMPTY(onLaunchDebugServer, Session &, std::string const &,
                 uint16_t &, ProcessId &)

DUMMY_IMPL_EMPTY_CONST(onQueryLaunchSuccess, Session &, ProcessId)

DUMMY_IMPL_EMPTY_CONST(onQueryUserName, Session &, UserId const &,
                       std::string &)

DUMMY_IMPL_EMPTY_CONST(onQueryGroupName, Session &, GroupId const &,
                       std::string &)

DUMMY_IMPL_EMPTY_CONST(onQueryWorkingDirectory, Session &,
                       std::string &woringDir)

DUMMY_IMPL_EMPTY(onReset, Session &)

DUMMY_IMPL_EMPTY(onFlashErase, Session &, Address const &, size_t)

DUMMY_IMPL_EMPTY(onFlashWrite, Session &, Address const &, ByteVector const &)

DUMMY_IMPL_EMPTY(onFlashDone, Session &)

DUMMY_IMPL_EMPTY(fetchStopInfoForAllThreads, Session &,
                 std::vector<StopInfo> &stops, StopInfo &processStop)

DUMMY_IMPL_EMPTY(createThreadsStopInfo, Session &, JSArray &threadsStopInfo)
} // namespace GDBRemote
} // namespace ds2
