//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_GDBRemote_PlatformSessionImpl_h
#define __DebugServer2_GDBRemote_PlatformSessionImpl_h

#include "DebugServer2/GDBRemote/DummySessionDelegateImpl.h"

namespace ds2 {
namespace GDBRemote {

class PlatformSessionImpl : public DummySessionDelegateImpl {
protected:
  typedef std::map<std::string, std::string> EnvironmentMap;

protected:
  // a struct to help iterate over the process list for onQueryProcessList
  mutable IdIterationState<ProcessId> _pids;

protected:
  bool _disableASLR;
  std::string _workingDirectory;
  std::string _stdFile[3];
  StringCollection _arguments;
  EnvironmentMap _environment;

public:
  PlatformSessionImpl();

protected:
  ErrorCode onQueryProcessList(Session &session, ProcessInfoMatch const &match,
                               bool first, ProcessInfo &info) const override;
  ErrorCode onQueryProcessInfo(Session &session, ProcessId pid,
                               ProcessInfo &info) const override;

  ErrorCode onExecuteProgram(Session &session, std::string const &command,
                             uint32_t timeout,
                             std::string const &workingDirectory,
                             ProgramResult &result) override;

  ErrorCode onFileOpen(Session &session, std::string const &path,
                       uint32_t flags, uint32_t mode, int &fd) override;
  ErrorCode onFileClose(Session &session, int fd) override;

  ErrorCode onFileExists(Session &session, std::string const &path) override;

protected:
  ErrorCode onQueryUserName(Session &session, UserId const &uid,
                            std::string &name) const override;
  ErrorCode onQueryGroupName(Session &session, GroupId const &gid,
                             std::string &name) const override;
  ErrorCode onQueryWorkingDirectory(Session &session,
                                    std::string &workingDir) const override;

protected:
  ErrorCode onLaunchDebugServer(Session &session, std::string const &host,
                                uint16_t &port, ProcessId &pid) override;

protected:
  ErrorCode onDisableASLR(Session &session, bool disable) override;
  ErrorCode onSetEnvironmentVariable(Session &session, std::string const &name,
                                     std::string const &value) override;
  ErrorCode onSetWorkingDirectory(Session &session,
                                  std::string const &path) override;
  ErrorCode onSetStdFile(Session &session, int fileno,
                         std::string const &path) override;
  ErrorCode onSetArchitecture(Session &session,
                              std::string const &architecture) override;
  ErrorCode onSetProgramArguments(Session &session,
                                  StringCollection const &args) override;
  ErrorCode onQueryLaunchSuccess(Session &session,
                                 ProcessId pid) const override;

private:
  void updateProcesses(ProcessInfoMatch const &match) const;
};
}
}

#endif // !__DebugServer2_GDBRemote_PlatformSessionImpl_h
