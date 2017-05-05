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

#include "DebugServer2/GDBRemote/Types.h"

namespace ds2 {
namespace GDBRemote {

class Session;

class SessionDelegate {
protected:
  friend class Session;

public:
  virtual ~SessionDelegate() = default;

protected: // General Information
  virtual size_t getGPRSize() const = 0;

protected: // Common
  virtual ErrorCode onEnableExtendedMode(Session &session) = 0;
  virtual ErrorCode onSetBaudRate(Session &session, uint32_t speed) = 0;
  virtual ErrorCode onToggleDebugFlag(Session &session) = 0;

  virtual ErrorCode onSetMaxPacketSize(Session &session, size_t size) = 0;
  virtual ErrorCode onSetMaxPayloadSize(Session &session, size_t size) = 0;

  virtual ErrorCode onSetLogging(Session &session, std::string const &mode,
                                 std::string const &filename,
                                 StringCollection const &flags) = 0;
  virtual ErrorCode onSendInput(Session &session, ByteVector const &buf) = 0;
  virtual ErrorCode
  onAllowOperations(Session &session,
                    std::map<std::string, bool> const &operations) = 0;
  virtual ErrorCode
  onQuerySupported(Session &session, Feature::Collection const &remoteFeatures,
                   Feature::Collection &localFeatures) const = 0;

  virtual ErrorCode onExecuteCommand(Session &session,
                                     std::string const &command) = 0;

  virtual ErrorCode onQueryServerVersion(Session &session,
                                         ServerVersion &version) const = 0;
  virtual ErrorCode onQueryHostInfo(Session &session, HostInfo &info) const = 0;

  virtual ErrorCode onQueryFileLoadAddress(Session &session,
                                           std::string const &file_path,
                                           Address &address) = 0;

protected: // Debugging Session
  virtual ErrorCode onEnableControlAgent(Session &session, bool enable) = 0;
  virtual ErrorCode onNonStopMode(Session &session, bool enable) = 0;
  virtual ErrorCode onEnableBTSTracing(Session &session, bool enable) = 0;

  virtual ErrorCode onPassSignals(Session &session,
                                  std::vector<int> const &signals) = 0;
  virtual ErrorCode onProgramSignals(Session &session,
                                     std::vector<int> const &signals) = 0;

  virtual ErrorCode onQuerySymbol(Session &session, std::string const &name,
                                  std::string const &value,
                                  std::string &next) const = 0;

  virtual ErrorCode onQueryRegisterInfo(Session &session, uint32_t regno,
                                        RegisterInfo &info) const = 0;

  virtual ErrorCode onAttach(Session &session, ProcessId pid, AttachMode mode,
                             StopInfo &stop) = 0;
  virtual ErrorCode onAttach(Session &session, std::string const &name,
                             AttachMode mode, StopInfo &stop) = 0;
  virtual ErrorCode onRunAttach(Session &session, std::string const &filename,
                                StringCollection const &arguments,
                                StopInfo &stop) = 0;
  virtual ErrorCode onDetach(Session &session, ProcessId pid, bool stopped) = 0;
  virtual ErrorCode onQueryAttached(Session &session, ProcessId pid,
                                    bool &attachedProcess) const = 0;
  virtual ErrorCode onQueryProcessInfo(Session &session,
                                       ProcessInfo &info) const = 0;
  virtual ErrorCode onQueryThreadStopInfo(Session &session,
                                          ProcessThreadId const &ptid,
                                          StopInfo &stop) const = 0;

  virtual ErrorCode onQueryHardwareWatchpointCount(Session &session,
                                                   size_t &count) const = 0;

  virtual ErrorCode onQuerySectionOffsets(Session &session, Address &text,
                                          Address &data,
                                          bool &isSegment) const = 0;
  virtual ErrorCode
  onQuerySharedLibrariesInfoAddress(Session &session,
                                    Address &address) const = 0;
  virtual ErrorCode onQuerySharedLibraryInfo(Session &session,
                                             std::string const &path,
                                             std::string const &triple,
                                             SharedLibraryInfo &info) const = 0;

  virtual ErrorCode onRestart(Session &session, ProcessId pid) = 0;
  virtual ErrorCode onInterrupt(Session &session) = 0;
  virtual ErrorCode onTerminate(Session &session, ProcessThreadId const &ptid,
                                StopInfo &stop) = 0;
  virtual ErrorCode onExitServer(Session &session) = 0;

  virtual ErrorCode onSynchronizeThreadState(Session &session,
                                             ProcessId pid) = 0;

  //
  // If lastTid is kAllThreadId, it's the first request; if it's kAnyThreadId
  // the next of the previous request, in any other case the thread next to
  // the one specified.
  //
  virtual ErrorCode onQueryThreadList(Session &session, ProcessId pid,
                                      ThreadId lastTid,
                                      ThreadId &tid) const = 0;

  virtual ErrorCode onQueryCurrentThread(Session &session,
                                         ProcessThreadId &ptid) const = 0;
  virtual ErrorCode onThreadIsAlive(Session &session,
                                    ProcessThreadId const &ptid) = 0;
  virtual ErrorCode onQueryThreadInfo(Session &session,
                                      ProcessThreadId const &ptid,
                                      uint32_t mode, void *info) const = 0;

  virtual ErrorCode onQueryTLSAddress(Session &session,
                                      ProcessThreadId const &ptid,
                                      Address const &offset,
                                      Address const &linkMap,
                                      Address &address) const = 0;
  virtual ErrorCode onQueryTIBAddress(Session &session,
                                      ProcessThreadId const &ptid,
                                      Address &address) const = 0;

  virtual ErrorCode onEnableAsynchronousProfiling(Session &session,
                                                  ProcessThreadId const &ptid,
                                                  bool enabled,
                                                  uint32_t interval,
                                                  uint32_t scanType) = 0;
  virtual ErrorCode onQueryProfileData(Session &session,
                                       ProcessThreadId const &ptid,
                                       uint32_t scanType, void *data) const = 0;

  virtual ErrorCode onResume(Session &session,
                             ThreadResumeAction::Collection const &actions,
                             StopInfo &stop) = 0;

  virtual ErrorCode
  onReadGeneralRegisters(Session &session, ProcessThreadId const &ptid,
                         Architecture::GPRegisterValueVector &regs) = 0;
  virtual ErrorCode
  onWriteGeneralRegisters(Session &session, ProcessThreadId const &ptid,
                          std::vector<uint64_t> const &regs) = 0;

  virtual ErrorCode onSaveRegisters(Session &session,
                                    ProcessThreadId const &ptid,
                                    uint64_t &id) = 0;
  virtual ErrorCode onRestoreRegisters(Session &session,
                                       ProcessThreadId const &ptid,
                                       uint64_t id) = 0;

  virtual ErrorCode onReadRegisterValue(Session &session,
                                        ProcessThreadId const &ptid,
                                        uint32_t regno, std::string &value) = 0;
  virtual ErrorCode onWriteRegisterValue(Session &session,
                                         ProcessThreadId const &ptid,
                                         uint32_t regno,
                                         std::string const &value) = 0;

  virtual ErrorCode onReadMemory(Session &session, Address const &address,
                                 size_t length, ByteVector &data) = 0;
  virtual ErrorCode onWriteMemory(Session &session, Address const &address,
                                  ByteVector const &data, size_t &nwritten) = 0;

  virtual ErrorCode onAllocateMemory(Session &session, size_t size,
                                     uint32_t permissions,
                                     Address &address) = 0;
  virtual ErrorCode onDeallocateMemory(Session &session,
                                       Address const &address) = 0;
  virtual ErrorCode onQueryMemoryRegionInfo(Session &session,
                                            Address const &address,
                                            MemoryRegionInfo &info) const = 0;

  virtual ErrorCode onComputeCRC(Session &session, Address const &address,
                                 size_t length, uint32_t &crc) = 0;

  virtual ErrorCode onSearch(Session &session, Address const &address,
                             std::string const &pattern, Address &location) = 0;
  virtual ErrorCode onSearchBackward(Session &session, Address const &address,
                                     uint32_t pattern, uint32_t mask,
                                     Address &location) = 0;

  virtual ErrorCode onInsertBreakpoint(Session &session, BreakpointType type,
                                       Address const &address, uint32_t kind,
                                       StringCollection const &conditions,
                                       StringCollection const &commands,
                                       bool persistentCommands) = 0;
  virtual ErrorCode onRemoveBreakpoint(Session &session, BreakpointType type,
                                       Address const &address,
                                       uint32_t kind) = 0;

  virtual ErrorCode onXferRead(Session &session, std::string const &object,
                               std::string const &annex, uint64_t offset,
                               uint64_t length, std::string &buffer,
                               bool &last) = 0;
  virtual ErrorCode onXferWrite(Session &session, std::string const &object,
                                std::string const &annex, uint64_t offset,
                                std::string const &buffer,
                                size_t &nwritten) = 0;

protected:
  virtual ErrorCode fetchStopInfoForAllThreads(Session &session,
                                               std::vector<StopInfo> &stops,
                                               StopInfo &processStop) = 0;
  virtual ErrorCode createThreadsStopInfo(Session &session,
                                          JSArray &threadsStopInfo) = 0;

protected: // Platform Session
  virtual ErrorCode onDisableASLR(Session &session, bool disable) = 0;

  virtual ErrorCode onSetEnvironmentVariable(Session &session,
                                             std::string const &name,
                                             std::string const &value) = 0;
  virtual ErrorCode onSetWorkingDirectory(Session &session,
                                          std::string const &path) = 0;
  virtual ErrorCode onSetStdFile(Session &session, int fileno,
                                 std::string const &path) = 0;

  virtual ErrorCode onSetArchitecture(Session &session,
                                      std::string const &architecture) = 0;

  virtual ErrorCode onSetProgramArguments(Session &session,
                                          StringCollection const &args) = 0;

  virtual ErrorCode onExecuteProgram(Session &session,
                                     std::string const &command,
                                     uint32_t timeout,
                                     std::string const &workingDirectory,
                                     ProgramResult &result) = 0;

  virtual ErrorCode onFileCreateDirectory(Session &session,
                                          std::string const &path,
                                          uint32_t mode) = 0;

  virtual ErrorCode onFileOpen(Session &session, std::string const &path,
                               OpenFlags flags, uint32_t mode, int &fd) = 0;
  virtual ErrorCode onFileClose(Session &session, int fd) = 0;
  virtual ErrorCode onFileRead(Session &session, int fd, uint64_t &count,
                               uint64_t offset, ByteVector &buffer) = 0;
  virtual ErrorCode onFileWrite(Session &session, int fd, uint64_t offset,
                                ByteVector const &buffer,
                                uint64_t &nwritten) = 0;

  virtual ErrorCode onFileRemove(Session &session, std::string const &path) = 0;
  virtual ErrorCode onFileReadLink(Session &session, std::string const &path,
                                   std::string &resolved) = 0;
  virtual ErrorCode onFileSetPermissions(Session &session,
                                         std::string const &path,
                                         uint32_t mode) = 0;
  virtual ErrorCode onFileExists(Session &session, std::string const &path) = 0;
  virtual ErrorCode onFileComputeMD5(Session &session, std::string const &path,
                                     uint8_t digest[16]) = 0;
  virtual ErrorCode onFileGetSize(Session &session, std::string const &path,
                                  uint64_t &size) = 0;

  virtual ErrorCode onQueryProcessList(Session &session,
                                       ProcessInfoMatch const &match,
                                       bool first, ProcessInfo &info) const = 0;
  virtual ErrorCode onQueryProcessInfo(Session &session, ProcessId pid,
                                       ProcessInfo &info) const = 0;

  virtual ErrorCode onLaunchDebugServer(Session &session,
                                        std::string const &host, uint16_t &port,
                                        ProcessId &pid) = 0;

  virtual ErrorCode onQueryLaunchSuccess(Session &session,
                                         ProcessId pid) const = 0;

  virtual ErrorCode onQueryUserName(Session &session, UserId const &uid,
                                    std::string &name) const = 0;
  virtual ErrorCode onQueryGroupName(Session &session, GroupId const &gid,
                                     std::string &name) const = 0;
  virtual ErrorCode onQueryWorkingDirectory(Session &session,
                                            std::string &workingDir) const = 0;

protected: // System Session
  virtual ErrorCode onReset(Session &session) = 0;
  virtual ErrorCode onFlashErase(Session &session, Address const &address,
                                 size_t length) = 0;
  virtual ErrorCode onFlashWrite(Session &session, Address const &address,
                                 ByteVector const &data) = 0;
  virtual ErrorCode onFlashDone(Session &session) = 0;
};
} // namespace GDBRemote
} // namespace ds2
