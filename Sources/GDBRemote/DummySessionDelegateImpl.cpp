//
// Copyright (c) 2014, Facebook, Inc.
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

DummySessionDelegateImpl::DummySessionDelegateImpl() : _secure(true) {}

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

ErrorCode DummySessionDelegateImpl::onSetMaxPacketSize(Session &, size_t) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onSetMaxPayloadSize(Session &, size_t) {
  return kErrorUnsupported;
}

void DummySessionDelegateImpl::onSetLogging(Session &, std::string const &,
                                            std::string const &,
                                            StringCollection const &) {}

ErrorCode DummySessionDelegateImpl::onAllowOperations(
    Session &, std::map<std::string, bool> const &) {
  return kSuccess;
}

ErrorCode DummySessionDelegateImpl::onQuerySupported(
    Session &, Feature::Collection const &, Feature::Collection &) {
  return kSuccess;
}

ErrorCode
DummySessionDelegateImpl::onExecuteCommand(Session &session,
                                           std::string const &command) {
  if (!_secure)
    return kErrorUnsupported;

  //
  // Need to inject back to the interpreter the command.
  //
  session.interpreter().onPacketData(command, true);
  return kSuccess;
}

ErrorCode
DummySessionDelegateImpl::onQueryServerVersion(Session &,
                                               ServerVersion &version) {
  // TODO use #defines
  version.name = "DebugServer2";
  version.version = "1.0.0";
  version.releaseName = "turin";
  version.majorVersion = 1;
  version.minorVersion = 0;
  version.buildNumber = 0;

  return kSuccess;
}

ErrorCode DummySessionDelegateImpl::onQueryHostInfo(Session &, HostInfo &info) {
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

  return kSuccess;
}

ErrorCode DummySessionDelegateImpl::onEnableControlAgent(Session &, bool) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onNonStopMode(Session &, bool) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onEnableBTSTracing(Session &, bool) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onPassSignals(Session &,
                                                  std::vector<int> const &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onProgramSignals(Session &,
                                                     std::vector<int> const &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onQuerySymbol(Session &,
                                                  std::string const &,
                                                  std::string const &,
                                                  std::string &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onQueryRegisterInfo(Session &, uint32_t,
                                                        RegisterInfo &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onAttach(Session &, ProcessId, AttachMode,
                                             StopCode &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onAttach(Session &, std::string const &,
                                             AttachMode, StopCode &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onRunAttach(Session &, std::string const &,
                                                StringCollection const &,
                                                StopCode &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onDetach(Session &, ProcessId, bool) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onQueryAttached(Session &, ProcessId,
                                                    bool &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onQueryProcessInfo(Session &,
                                                       ProcessInfo &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onQueryHardwareWatchpointCount(Session &,
                                                                   size_t &) {
  return kSuccess;
}

ErrorCode DummySessionDelegateImpl::onQuerySectionOffsets(Session &, Address &,
                                                          Address &, bool &) {
  return kSuccess;
}

ErrorCode
DummySessionDelegateImpl::onQuerySharedLibrariesInfoAddress(Session &,
                                                            Address &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onQuerySharedLibraryInfo(
    Session &session, std::string const &path, std::string const &triple,
    SharedLibraryInfo &info) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onRestart(Session &, ProcessId) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onInterrupt(Session &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onTerminate(Session &,
                                                ProcessThreadId const &,
                                                StopCode &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onTerminate(Session &session,
                                                ProcessId pid) {
  StopCode stop;
  return onTerminate(session, pid, stop);
}

ErrorCode DummySessionDelegateImpl::onSynchronizeThreadState(Session &,
                                                             ProcessId) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onQueryThreadList(Session &, ProcessId,
                                                      ThreadId, ThreadId &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onQueryCurrentThread(Session &,
                                                         ProcessThreadId &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onQueryThreadStopInfo(
    Session &, ProcessThreadId const &, StopCode &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onThreadIsAlive(Session &,
                                                    ProcessThreadId const &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onQueryThreadInfo(Session &,
                                                      ProcessThreadId const &,
                                                      uint32_t, void *) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onQueryTLSAddress(Session &,
                                                      ProcessThreadId const &,
                                                      Address const &,
                                                      Address const &,
                                                      Address &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onQueryTIBAddress(Session &,
                                                      ProcessThreadId const &,
                                                      Address &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onEnableAsynchronousProfiling(
    Session &, ProcessThreadId const &, bool, uint32_t, uint32_t) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onQueryProfileData(Session &,
                                                       ProcessThreadId const &,
                                                       uint32_t, void *) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onResume(
    Session &, ThreadResumeAction::Collection const &, StopCode &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onReadGeneralRegisters(
    Session &, ProcessThreadId const &, Architecture::GPRegisterValueVector &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onWriteGeneralRegisters(
    Session &, ProcessThreadId const &, std::vector<uint64_t> const &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onReadRegisterValue(Session &,
                                                        ProcessThreadId const &,
                                                        uint32_t,
                                                        std::string &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onWriteRegisterValue(
    Session &, ProcessThreadId const &, uint32_t, std::string const &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onSaveRegisters(Session &session,
                                                    ProcessThreadId const &ptid,
                                                    uint64_t &id) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onRestoreRegisters(
    Session &session, ProcessThreadId const &ptid, uint64_t id) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onReadMemory(Session &, Address const &,
                                                 size_t, std::string &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onWriteMemory(Session &, Address const &,
                                                  std::string const &,
                                                  size_t &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onAllocateMemory(Session &, size_t,
                                                     uint32_t, Address &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onDeallocateMemory(Session &,
                                                       Address const &) {
  return kErrorUnsupported;
}

ErrorCode
DummySessionDelegateImpl::onQueryMemoryRegionInfo(Session &, Address const &,
                                                  MemoryRegionInfo &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onComputeCRC(Session &, Address const &,
                                                 size_t, uint32_t &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onSearchBackward(Session &, Address const &,
                                                     uint32_t, uint32_t,
                                                     Address &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onSearch(Session &, Address const &,
                                             std::string const &, Address &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onInsertBreakpoint(
    Session &, BreakpointType, Address const &, uint32_t,
    StringCollection const &, StringCollection const &, bool) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onRemoveBreakpoint(Session &,
                                                       BreakpointType,
                                                       Address const &,
                                                       uint32_t) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onXferRead(Session &, std::string const &,
                                               std::string const &, uint64_t,
                                               uint64_t, std::string &,
                                               bool &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onXferWrite(Session &, std::string const &,
                                                std::string const &, uint64_t,
                                                std::string const &, size_t &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onDisableASLR(Session &, bool) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onSetEnvironmentVariable(
    Session &, std::string const &, std::string const &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onSetWorkingDirectory(Session &,
                                                          std::string const &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onSetStdFile(Session &, int,
                                                 std::string const &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onSetArchitecture(Session &,
                                                      std::string const &) {
  return kErrorUnsupported;
}

ErrorCode
DummySessionDelegateImpl::onSetProgramArguments(Session &,
                                                StringCollection const &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onExecuteProgram(Session &,
                                                     std::string const &,
                                                     uint32_t,
                                                     std::string const &,
                                                     ProgramResult &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onFileCreateDirectory(Session &,
                                                          std::string const &,
                                                          uint32_t) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onFileOpen(Session &, std::string const &,
                                               uint32_t, uint32_t, int &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onFileClose(Session &, int) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onFileRead(Session &, int, size_t, uint64_t,
                                               std::string &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onFileWrite(Session &, int, uint64_t,
                                                std::string const &, size_t &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onFileRemove(Session &,
                                                 std::string const &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onFileReadLink(Session &,
                                                   std::string const &,
                                                   std::string &) {
  return kErrorUnsupported;
}

#if 0
//
// more F packets:
// https://sourceware.org/gdb/onlinedocs/gdb/List-of-Supported-Calls.html#List-of-Supported-Calls
//
ErrorCode DummySessionDelegateImpl::
onGetCurrentTime(Session &, TimeValue &)
{
    return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::
onFileIsATTY(Session &, int)
{
    return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::
onFileRename(Session &, std::string const &, std::string const &)
{
    return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::
onFileGetStat(Session &, std::string const &, FileStat &)
{
    return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::
onFileGetStat(Session &, int, FileStat &)
{
    return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::
onFileSeek(Session &, int, int64_t, int, int64_t &)
{
    return kErrorUnsupported;
}
#endif

ErrorCode DummySessionDelegateImpl::onFileExists(Session &,
                                                 std::string const &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onFileComputeMD5(Session &,
                                                     std::string const &,
                                                     uint8_t[16]) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onFileGetSize(Session &,
                                                  std::string const &,
                                                  uint64_t &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onQueryProcessList(Session &,
                                                       ProcessInfoMatch const &,
                                                       bool, ProcessInfo &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onQueryProcessInfo(Session &, ProcessId,
                                                       ProcessInfo &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onLaunchDebugServer(Session &,
                                                        std::string const &,
                                                        uint16_t &,
                                                        ProcessId &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onQueryLaunchSuccess(Session &, ProcessId) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onQueryUserName(Session &, UserId const &,
                                                    std::string &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onQueryGroupName(Session &, GroupId const &,
                                                     std::string &) {
  return kErrorUnsupported;
}

ErrorCode
DummySessionDelegateImpl::onQueryWorkingDirectory(Session &,
                                                  std::string &woringDir) {
  return kErrorUnsupported;
}

void DummySessionDelegateImpl::onReset(Session &) {}

ErrorCode DummySessionDelegateImpl::onFlashErase(Session &, Address const &,
                                                 size_t) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onFlashWrite(Session &, Address const &,
                                                 std::string const &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::onFlashDone(Session &) {
  return kErrorUnsupported;
}

ErrorCode DummySessionDelegateImpl::fetchStopInfoForAllThreads(
    Session &session, std::vector<StopCode> &stops, StopCode &processStop) {
  return kErrorUnsupported;
}

ErrorCode
DummySessionDelegateImpl::createThreadsStopInfo(Session &session,
                                                JSArray &threadsStopInfo) {
  return kErrorUnsupported;
}
}
}
