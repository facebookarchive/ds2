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

#include "DebugServer2/GDBRemote/DummySessionDelegateImpl.h"
#include "DebugServer2/GDBRemote/Mixins/FileOperationsMixin.h"
#include "DebugServer2/Host/ProcessSpawner.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Utils/MPL.h"

#include <mutex>

namespace ds2 {
namespace GDBRemote {
class DebugSessionImplBase : public DummySessionDelegateImpl {
protected:
  Target::Process *_process;
  std::vector<int> _programmedSignals;
  std::map<uint64_t, size_t> _allocations;
  std::map<uint64_t, Architecture::CPUState> _savedRegisters;
  Host::ProcessSpawner _spawner;

protected:
  // a struct to help iterate over the thread list for onQueryThreadList
  mutable IterationState<ThreadId> _threadIterationState;

protected:
  std::mutex _resumeSessionLock;
  Session *_resumeSession;
  std::string _consoleBuffer;

public:
  DebugSessionImplBase(StringCollection const &args,
                       EnvironmentBlock const &env);
  DebugSessionImplBase(int attachPid);
  DebugSessionImplBase();
  ~DebugSessionImplBase() override;

protected:
  size_t getGPRSize() const override;

protected:
  ErrorCode onInterrupt(Session &session) override;

protected:
  ErrorCode onQuerySupported(Session &session,
                             Feature::Collection const &remoteFeatures,
                             Feature::Collection &localFeatures) const override;
  ErrorCode onPassSignals(Session &session,
                          std::vector<int> const &signals) override;
  ErrorCode onProgramSignals(Session &session,
                             std::vector<int> const &signals) override;
  ErrorCode onNonStopMode(Session &session, bool enable) override;
  ErrorCode onSendInput(Session &session, ByteVector const &buf) override;

protected:
  ErrorCode onQueryCurrentThread(Session &session,
                                 ProcessThreadId &ptid) const override;
  ErrorCode onThreadIsAlive(Session &session,
                            ProcessThreadId const &ptid) override;
  ErrorCode onQueryAttached(Session &session, ProcessId pid,
                            bool &attachedProcess) const override;
  ErrorCode onQueryProcessInfo(Session &session,
                               ProcessInfo &info) const override;
  ErrorCode onQueryHardwareWatchpointCount(Session &session,
                                           size_t &count) const override;
  ErrorCode onQueryThreadStopInfo(Session &session, ProcessThreadId const &ptid,
                                  StopInfo &stop) const override;

  ErrorCode onQueryThreadList(Session &session, ProcessId pid, ThreadId lastTid,
                              ThreadId &tid) const override;

  ErrorCode onQueryFileLoadAddress(Session &session,
                                   std::string const &file_path,
                                   Address &address) override;

protected:
  ErrorCode onQueryRegisterInfo(Session &session, uint32_t regno,
                                RegisterInfo &info) const override;

protected:
  ErrorCode onQuerySharedLibrariesInfoAddress(Session &session,
                                              Address &address) const override;

protected:
  ErrorCode onXferRead(Session &session, std::string const &object,
                       std::string const &annex, uint64_t offset,
                       uint64_t length, std::string &buffer,
                       bool &last) override;

protected:
  ErrorCode onSetStdFile(Session &session, int fileno,
                         std::string const &path) override;

protected:
  ErrorCode
  onReadGeneralRegisters(Session &session, ProcessThreadId const &ptid,
                         Architecture::GPRegisterValueVector &regs) override;
  ErrorCode onWriteGeneralRegisters(Session &session,
                                    ProcessThreadId const &ptid,
                                    std::vector<uint64_t> const &regs) override;

  ErrorCode onSaveRegisters(Session &session, ProcessThreadId const &ptid,
                            uint64_t &id) override;
  ErrorCode onRestoreRegisters(Session &session, ProcessThreadId const &ptid,
                               uint64_t id) override;

  ErrorCode onReadRegisterValue(Session &session, ProcessThreadId const &ptid,
                                uint32_t regno, std::string &value) override;
  ErrorCode onWriteRegisterValue(Session &session, ProcessThreadId const &ptid,
                                 uint32_t regno,
                                 std::string const &value) override;

protected:
  ErrorCode onReadMemory(Session &session, Address const &address,
                         size_t length, ByteVector &data) override;
  ErrorCode onWriteMemory(Session &session, Address const &address,
                          ByteVector const &data, size_t &nwritten) override;

  ErrorCode onAllocateMemory(Session &session, size_t size,
                             uint32_t permissions, Address &address) override;
  ErrorCode onDeallocateMemory(Session &session,
                               Address const &address) override;

  ErrorCode onQueryMemoryRegionInfo(Session &session, Address const &address,
                                    MemoryRegionInfo &info) const override;

protected:
  ErrorCode onSetEnvironmentVariable(Session &session, std::string const &name,
                                     std::string const &value) override;
  ErrorCode onSetProgramArguments(Session &session,
                                  StringCollection const &args) override;
  ErrorCode onQueryLaunchSuccess(Session &session,
                                 ProcessId pid) const override;

protected:
  ErrorCode onAttach(Session &session, ProcessId pid, AttachMode mode,
                     StopInfo &stop) override;

protected:
  ErrorCode onResume(Session &session,
                     ThreadResumeAction::Collection const &actions,
                     StopInfo &stop) override;
  ErrorCode onTerminate(Session &session, ProcessThreadId const &ptid,
                        StopInfo &stop) override;
  ErrorCode onDetach(Session &session, ProcessId pid, bool stopped) override;
  ErrorCode onExitServer(Session &session) override;

protected:
  ErrorCode onInsertBreakpoint(Session &session, BreakpointType type,
                               Address const &address, uint32_t size,
                               StringCollection const &conditions,
                               StringCollection const &commands,
                               bool persistentCommands) override;
  ErrorCode onRemoveBreakpoint(Session &session, BreakpointType type,
                               Address const &address, uint32_t kind) override;

protected:
  Target::Thread *findThread(ProcessThreadId const &ptid) const;
  ErrorCode queryStopInfo(Session &session, Target::Thread *thread,
                          StopInfo &stop) const;
  ErrorCode queryStopInfo(Session &session, ProcessThreadId const &ptid,
                          StopInfo &stop) const;

protected:
  ErrorCode fetchStopInfoForAllThreads(Session &session,
                                       std::vector<StopInfo> &stops,
                                       StopInfo &processStop) override;
  ErrorCode createThreadsStopInfo(Session &session,
                                  JSArray &threadsStopInfo) override;

private:
  ErrorCode spawnProcess(StringCollection const &args,
                         EnvironmentBlock const &env);
  void appendOutput(char const *buf, size_t size);
};

using DebugSessionImpl =
    Utils::MixinApply<DebugSessionImplBase, FileOperationsMixin>;
} // namespace GDBRemote
} // namespace ds2
