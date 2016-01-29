//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_GDBRemote_DebugSessionImpl_h
#define __DebugServer2_GDBRemote_DebugSessionImpl_h

#include "DebugServer2/GDBRemote/DummySessionDelegateImpl.h"
#include "DebugServer2/Host/ProcessSpawner.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Target/Thread.h"

#include <mutex>

namespace ds2 {
namespace GDBRemote {
class DebugSessionImpl : public DummySessionDelegateImpl {
protected:
  Target::Process *_process;
  std::vector<ThreadId> _tids;
  std::vector<int> _programmedSignals;
  std::map<uint64_t, size_t> _allocations;
  std::map<uint64_t, Architecture::CPUState> _savedRegisters;
  size_t _threadIndex;
  Host::ProcessSpawner _spawner;

protected:
  std::mutex _resumeSessionLock;
  Session *_resumeSession;
  std::string _consoleBuffer;

public:
  DebugSessionImpl(StringCollection const &args, EnvironmentBlock const &env);
  DebugSessionImpl(int attachPid);
  DebugSessionImpl();
  ~DebugSessionImpl() override;

protected:
  size_t getGPRSize() const override;

protected:
  ErrorCode onInterrupt(Session &session) override;

protected:
  ErrorCode onQuerySupported(Session &session,
                             Feature::Collection const &remoteFeatures,
                             Feature::Collection &localFeatures) override;
  ErrorCode onPassSignals(Session &session,
                          std::vector<int> const &signals) override;
  ErrorCode onProgramSignals(Session &session,
                             std::vector<int> const &signals) override;
  ErrorCode onNonStopMode(Session &session, bool enable) override;

protected:
  ErrorCode onQueryCurrentThread(Session &session,
                                 ProcessThreadId &ptid) override;
  ErrorCode onThreadIsAlive(Session &session,
                            ProcessThreadId const &ptid) override;
  ErrorCode onQueryAttached(Session &session, ProcessId pid,
                            bool &attachedProcess) override;
  ErrorCode onQueryProcessInfo(Session &session, ProcessInfo &info) override;
  ErrorCode onQueryThreadStopInfo(Session &session, ProcessThreadId const &ptid,
                                  StopCode &stop) override;

  ErrorCode onQueryThreadList(Session &session, ProcessId pid, ThreadId lastTid,
                              ThreadId &tid) override;

protected:
  ErrorCode onQueryRegisterInfo(Session &session, uint32_t regno,
                                RegisterInfo &info) override;

protected:
  ErrorCode onQuerySharedLibrariesInfoAddress(Session &session,
                                              Address &address) override;

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
                         size_t length, std::string &data) override;
  ErrorCode onWriteMemory(Session &session, Address const &address,
                          std::string const &data, size_t &nwritten) override;

  ErrorCode onAllocateMemory(Session &session, size_t size,
                             uint32_t permissions, Address &address) override;
  ErrorCode onDeallocateMemory(Session &session,
                               Address const &address) override;

  ErrorCode onQueryMemoryRegionInfo(Session &session, Address const &address,
                                    MemoryRegionInfo &info) override;

protected:
  ErrorCode onSetEnvironmentVariable(Session &session, std::string const &name,
                                     std::string const &value) override;
  ErrorCode onSetProgramArguments(Session &session,
                                  StringCollection const &args) override;
  ErrorCode onQueryLaunchSuccess(Session &session, ProcessId pid) override;

protected:
  ErrorCode onAttach(Session &session, ProcessId pid, AttachMode mode,
                     StopCode &stop) override;

protected:
  ErrorCode onResume(Session &session,
                     ThreadResumeAction::Collection const &actions,
                     StopCode &stop) override;
  ErrorCode onTerminate(Session &session, ProcessThreadId const &ptid,
                        StopCode &stop) override;
  ErrorCode onDetach(Session &session, ProcessId pid, bool stopped) override;

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
  ErrorCode queryStopCode(Session &session, ProcessThreadId const &ptid,
                          StopCode &stop) const;

private:
  ErrorCode spawnProcess(StringCollection const &args,
                         EnvironmentBlock const &env);
};
}
}

#endif // !__DebugServer2_GDBRemote_DebugSessionImpl_h
