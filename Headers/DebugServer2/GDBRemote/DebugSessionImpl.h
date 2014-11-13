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

namespace ds2 {
namespace GDBRemote {
class DebugSessionImpl : public DummySessionDelegateImpl {
protected:
  Target::Process *_process;
  std::vector<ThreadId> _tids;
  std::vector<int> _programmedSignals;
  std::map<uint64_t, size_t> _allocations;
  size_t _threadIndex;
  Host::ProcessSpawner _spawner;
  Session *_resumeSession;

public:
  DebugSessionImpl(int argc, char **argv);
  DebugSessionImpl(int attachPid);
  DebugSessionImpl();
  ~DebugSessionImpl();

protected:
  virtual size_t getGPRSize() const;

protected:
  virtual ErrorCode onInterrupt(Session &session);

protected:
  virtual ErrorCode onQuerySupported(Session &session,
                                     Feature::Collection const &remoteFeatures,
                                     Feature::Collection &localFeatures);
  virtual ErrorCode onPassSignals(Session &session,
                                  std::vector<int> const &signals);
  virtual ErrorCode onProgramSignals(Session &session,
                                     std::vector<int> const &signals);
  virtual ErrorCode onNonStopMode(Session &session, bool enable);

protected:
  virtual ErrorCode onQueryCurrentThread(Session &session,
                                         ProcessThreadId &ptid);
  virtual ErrorCode onThreadIsAlive(Session &session,
                                    ProcessThreadId const &ptid);
  virtual ErrorCode onQueryAttached(Session &session, ProcessId pid,
                                    bool &attachedProcess);
  virtual ErrorCode onQueryProcessInfo(Session &session, ProcessInfo &info);
  virtual ErrorCode onQueryThreadStopInfo(Session &session,
                                          ProcessThreadId const &ptid,
                                          bool list, StopCode &stop);

  virtual ErrorCode onQueryThreadList(Session &session, ProcessId pid,
                                      ThreadId lastTid, ThreadId &tid);

protected:
  virtual ErrorCode onQueryRegisterInfo(Session &session, uint32_t regno,
                                        RegisterInfo &info);

protected:
  virtual ErrorCode onQuerySharedLibrariesInfoAddress(Session &session,
                                                      Address &address);

protected:
  virtual ErrorCode onXferRead(Session &session, std::string const &object,
                               std::string const &annex, uint64_t offset,
                               uint64_t length, std::string &buffer,
                               bool &last);

protected:
  virtual ErrorCode
  onReadGeneralRegisters(Session &session, ProcessThreadId const &ptid,
                         Architecture::GPRegisterValueVector &regs);
  virtual ErrorCode onWriteGeneralRegisters(Session &session,
                                            ProcessThreadId const &ptid,
                                            std::vector<uint64_t> const &regs);

  virtual ErrorCode onReadRegisterValue(Session &session,
                                        ProcessThreadId const &ptid,
                                        uint32_t regno, std::string &value);
  virtual ErrorCode onWriteRegisterValue(Session &session,
                                         ProcessThreadId const &ptid,
                                         uint32_t regno,
                                         std::string const &value);

protected:
  virtual ErrorCode onReadMemory(Session &session, Address const &address,
                                 size_t length, std::string &data);
  virtual ErrorCode onWriteMemory(Session &session, Address const &address,
                                  std::string const &data, size_t &nwritten);

  virtual ErrorCode onAllocateMemory(Session &session, size_t size,
                                     uint32_t permissions, Address &address);
  virtual ErrorCode onDeallocateMemory(Session &session,
                                       Address const &address);

protected:
  virtual ErrorCode onSetProgramArguments(Session &session,
                                          StringCollection const &args);
  virtual ErrorCode onQueryLaunchSuccess(Session &session, ProcessId pid);

protected:
  virtual ErrorCode onAttach(Session &session, ProcessId pid, AttachMode mode,
                             StopCode &stop);

protected:
  virtual ErrorCode onResume(Session &session,
                             ThreadResumeAction::Collection const &actions,
                             StopCode &stop);
  virtual ErrorCode onTerminate(Session &session, ProcessThreadId const &ptid,
                                StopCode &stop);
  virtual ErrorCode onDetach(Session &session, ProcessId pid, bool stopped);

protected:
  virtual ErrorCode onInsertBreakpoint(Session &session, BreakpointType type,
                                       Address const &address, uint32_t kind,
                                       StringCollection const &conditions,
                                       StringCollection const &commands,
                                       bool persistentCommands);
  virtual ErrorCode onRemoveBreakpoint(Session &session, BreakpointType type,
                                       Address const &address, uint32_t kind);

protected:
  Target::Thread *findThread(ProcessThreadId const &ptid) const;
  ErrorCode queryStopCode(Session &session, ProcessThreadId const &ptid,
                          StopCode &stop) const;

private:
  ErrorCode spawnProcess(std::string const &path, StringCollection const &args);
};
}
}

#endif // !__DebugServer2_GDBRemote_DebugSessionImpl_h
