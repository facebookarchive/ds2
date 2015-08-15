//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_GDBRemote_DummySessionDelegateImpl_h
#define __DebugServer2_GDBRemote_DummySessionDelegateImpl_h

#include "DebugServer2/GDBRemote/SessionDelegate.h"

namespace ds2 {
namespace GDBRemote {

class DummySessionDelegateImpl : public SessionDelegate {
protected:
  bool _secure;

protected:
  DummySessionDelegateImpl();

protected: // General Information
  virtual size_t getGPRSize() const;

protected: // Common
  virtual ErrorCode onEnableExtendedMode(Session &session);
  virtual ErrorCode onSetBaudRate(Session &session, uint32_t speed);
  virtual ErrorCode onToggleDebugFlag(Session &session);

  virtual ErrorCode onSetMaxPacketSize(Session &session, size_t size);
  virtual ErrorCode onSetMaxPayloadSize(Session &session, size_t size);

  virtual void onSetLogging(Session &session, std::string const &mode,
                            std::string const &filename,
                            StringCollection const &flags);

  virtual ErrorCode
  onAllowOperations(Session &session,
                    std::map<std::string, bool> const &operations);
  virtual ErrorCode onQuerySupported(Session &session,
                                     Feature::Collection const &remoteFeatures,
                                     Feature::Collection &localFeatures);

  virtual ErrorCode onExecuteCommand(Session &session,
                                     std::string const &command);

  virtual ErrorCode onQueryServerVersion(Session &session,
                                         ServerVersion &version);
  virtual ErrorCode onQueryHostInfo(Session &session, HostInfo &info);

protected: // Debugging Session
  virtual ErrorCode onEnableControlAgent(Session &session, bool enable);
  virtual ErrorCode onNonStopMode(Session &session, bool enable);
  virtual ErrorCode onEnableBTSTracing(Session &session, bool enable);

  virtual ErrorCode onPassSignals(Session &session,
                                  std::vector<int> const &signals);
  virtual ErrorCode onProgramSignals(Session &session,
                                     std::vector<int> const &signals);

  virtual ErrorCode onQuerySymbol(Session &session, std::string const &name,
                                  std::string const &value, std::string &next);

  virtual ErrorCode onQueryRegisterInfo(Session &session, uint32_t regno,
                                        RegisterInfo &info);

  virtual ErrorCode onAttach(Session &session, ProcessId pid, AttachMode mode,
                             StopCode &stop);
  virtual ErrorCode onAttach(Session &session, std::string const &name,
                             AttachMode mode, StopCode &stop);
  virtual ErrorCode onRunAttach(Session &session, std::string const &filename,
                                StringCollection const &arguments,
                                StopCode &stop);
  virtual ErrorCode onDetach(Session &session, ProcessId pid, bool stopped);
  virtual ErrorCode onQueryAttached(Session &session, ProcessId pid,
                                    bool &attachedProcess);
  virtual ErrorCode onQueryProcessInfo(Session &session, ProcessInfo &info);

  virtual ErrorCode onQueryHardwareWatchpointCount(Session &session,
                                                   size_t &count);

  virtual ErrorCode onQuerySectionOffsets(Session &session, Address &text,
                                          Address &data, bool &isSegment);
  virtual ErrorCode onQuerySharedLibrariesInfoAddress(Session &session,
                                                      Address &address);
  virtual ErrorCode onQuerySharedLibraryInfo(Session &session,
                                             std::string const &path,
                                             std::string const &triple,
                                             SharedLibraryInfo &info);

  virtual ErrorCode onRestart(Session &session, ProcessId pid);
  virtual ErrorCode onInterrupt(Session &session);
  virtual ErrorCode onTerminate(Session &session, ProcessThreadId const &ptid,
                                StopCode &stop);
  virtual ErrorCode onTerminate(Session &session, ProcessId pid);

  virtual ErrorCode onSynchronizeThreadState(Session &session, ProcessId pid);

  //
  // If lastTid is kAllThreadId, it's the first request; if it's kAnyThreadId
  // the next of the previous request, in any other case the thread next to
  // the one specified.
  //
  virtual ErrorCode onQueryThreadList(Session &session, ProcessId pid,
                                      ThreadId lastTid, ThreadId &tid);

  virtual ErrorCode onQueryThreadStopInfo(Session &session,
                                          ProcessThreadId const &ptid,
                                          bool list, StopCode &stop);

  virtual ErrorCode onQueryCurrentThread(Session &session,
                                         ProcessThreadId &ptid);
  virtual ErrorCode onThreadIsAlive(Session &session,
                                    ProcessThreadId const &ptid);
  virtual ErrorCode onQueryThreadInfo(Session &session,
                                      ProcessThreadId const &ptid,
                                      uint32_t mode, void *info);

  virtual ErrorCode onQueryTLSAddress(Session &session,
                                      ProcessThreadId const &ptid,
                                      Address const &offset,
                                      Address const &linkMap, Address &address);
  virtual ErrorCode onQueryTIBAddress(Session &session,
                                      ProcessThreadId const &ptid,
                                      Address &address);

  virtual ErrorCode onEnableAsynchronousProfiling(Session &session,
                                                  ProcessThreadId const &ptid,
                                                  bool enabled,
                                                  uint32_t interval,
                                                  uint32_t scanType);
  virtual ErrorCode onQueryProfileData(Session &session,
                                       ProcessThreadId const &ptid,
                                       uint32_t scanType, void *data);

  virtual ErrorCode onResume(Session &session,
                             ThreadResumeAction::Collection const &actions,
                             StopCode &stop);

  virtual ErrorCode
  onReadGeneralRegisters(Session &session, ProcessThreadId const &ptid,
                         Architecture::GPRegisterValueVector &regs);
  virtual ErrorCode onWriteGeneralRegisters(Session &session,
                                            ProcessThreadId const &ptid,
                                            std::vector<uint64_t> const &regs);

  virtual ErrorCode onSaveRegisters(Session &session,
                                    ProcessThreadId const &ptid, uint64_t &id);
  virtual ErrorCode onRestoreRegisters(Session &session,
                                       ProcessThreadId const &ptid,
                                       uint64_t id);

  virtual ErrorCode onReadRegisterValue(Session &session,
                                        ProcessThreadId const &ptid,
                                        uint32_t regno, std::string &value);
  virtual ErrorCode onWriteRegisterValue(Session &session,
                                         ProcessThreadId const &ptid,
                                         uint32_t regno,
                                         std::string const &value);

  virtual ErrorCode onReadMemory(Session &session, Address const &address,
                                 size_t length, std::string &data);
  virtual ErrorCode onWriteMemory(Session &session, Address const &address,
                                  std::string const &data, size_t &nwritten);

  virtual ErrorCode onAllocateMemory(Session &session, size_t size,
                                     uint32_t permissions, Address &address);
  virtual ErrorCode onDeallocateMemory(Session &session,
                                       Address const &address);
  virtual ErrorCode onQueryMemoryRegionInfo(Session &session,
                                            Address const &address,
                                            MemoryRegionInfo &info);

  virtual ErrorCode onComputeCRC(Session &session, Address const &address,
                                 size_t length, uint32_t &crc);

  virtual ErrorCode onSearch(Session &session, Address const &address,
                             std::string const &pattern, Address &location);
  virtual ErrorCode onSearchBackward(Session &session, Address const &address,
                                     uint32_t pattern, uint32_t mask,
                                     Address &location);

  virtual ErrorCode onInsertBreakpoint(Session &session, BreakpointType type,
                                       Address const &address, uint32_t kind,
                                       StringCollection const &conditions,
                                       StringCollection const &commands,
                                       bool persistentCommands);
  virtual ErrorCode onRemoveBreakpoint(Session &session, BreakpointType type,
                                       Address const &address, uint32_t kind);

  virtual ErrorCode onXferRead(Session &session, std::string const &object,
                               std::string const &annex, uint64_t offset,
                               uint64_t length, std::string &buffer,
                               bool &last);
  virtual ErrorCode onXferWrite(Session &session, std::string const &object,
                                std::string const &annex, uint64_t offset,
                                std::string const &buffer, size_t &nwritten);

protected: // Platform Session
  virtual ErrorCode onDisableASLR(Session &session, bool disable);

  virtual ErrorCode onSetEnvironmentVariable(Session &session,
                                             std::string const &name,
                                             std::string const &value);
  virtual ErrorCode onSetWorkingDirectory(Session &session,
                                          std::string const &path);
  virtual ErrorCode onSetStdFile(Session &session, int fileno,
                                 std::string const &path);

  virtual ErrorCode onSetArchitecture(Session &session,
                                      std::string const &architecture);

  virtual ErrorCode onSetProgramArguments(Session &session,
                                          StringCollection const &args);

  virtual ErrorCode onExecuteProgram(Session &session,
                                     std::string const &command,
                                     uint32_t timeout,
                                     std::string const &workingDirectory,
                                     ProgramResult &result);

  virtual ErrorCode onFileCreateDirectory(Session &session,
                                          std::string const &path,
                                          uint32_t mode);

  virtual ErrorCode onFileOpen(Session &session, std::string const &path,
                               uint32_t flags, uint32_t mode, int &fd);
  virtual ErrorCode onFileClose(Session &session, int fd);
  virtual ErrorCode onFileRead(Session &session, int fd, size_t count,
                               uint64_t offset, std::string &buffer);
  virtual ErrorCode onFileWrite(Session &session, int fd, uint64_t offset,
                                std::string const &buffer, size_t &nwritten);

  virtual ErrorCode onFileRemove(Session &session, std::string const &path);
  virtual ErrorCode onFileReadLink(Session &session, std::string const &path,
                                   std::string &resolved);

#if 0
    //
    // more F packets:
    // https://sourceware.org/gdb/onlinedocs/gdb/List-of-Supported-Calls.html#List-of-Supported-Calls
    //
    virtual ErrorCode onGetCurrentTime(Session &session, TimeValue &tv);

    virtual ErrorCode onFileIsATTY(Session &session, int fd);
    virtual ErrorCode onFileRename(Session &session,
            std::string const &oldPath, std::string const &newPath);

    virtual ErrorCode onFileGetStat(Session &session, std::string const &path,
            FileStat &stat);
    virtual ErrorCode onFileGetStat(Session &session, int fd,
            FileStat &stat);

    virtual ErrorCode onFileSeek(Session &session, int fd,
            int64_t offset, int whence, int64_t &newOffset);
#endif

  virtual ErrorCode onFileExists(Session &session, std::string const &path);
  virtual ErrorCode onFileComputeMD5(Session &session, std::string const &path,
                                     uint8_t digest[16]);
  virtual ErrorCode onFileGetSize(Session &session, std::string const &path,
                                  uint64_t &size);

  virtual ErrorCode onQueryProcessList(Session &session,
                                       ProcessInfoMatch const &match,
                                       bool first, ProcessInfo &info);
  virtual ErrorCode onQueryProcessInfo(Session &session, ProcessId pid,
                                       ProcessInfo &info);

  virtual ErrorCode onLaunchDebugServer(Session &session,
                                        std::string const &host, uint16_t &port,
                                        ProcessId &pid);

  virtual ErrorCode onQueryLaunchSuccess(Session &session, ProcessId pid);

  virtual ErrorCode onQueryUserName(Session &session, UserId const &uid,
                                    std::string &name);
  virtual ErrorCode onQueryGroupName(Session &session, GroupId const &gid,
                                     std::string &name);
  virtual ErrorCode onQueryWorkingDirectory(Session &session,
                                            std::string &workingDir);

protected: // System Session
  virtual void onReset(Session &session);
  virtual ErrorCode onFlashErase(Session &session, Address const &address,
                                 size_t length);
  virtual ErrorCode onFlashWrite(Session &session, Address const &address,
                                 std::string const &data);
  virtual ErrorCode onFlashDone(Session &session);
};
}
}

#endif // !__DebugServer2_GDBRemote_DummySessionDelegateImpl_h
