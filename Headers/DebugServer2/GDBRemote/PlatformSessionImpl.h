//
// Copyright (c) 2014, Facebook, Inc.
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
  ProcessInfo::Collection _processes;
  size_t _processIndex;

protected:
  bool _disableASLR;
  std::string _workingDirectory;
  std::string _stdFile[3];
  StringCollection _arguments;
  EnvironmentMap _environment;

public:
  PlatformSessionImpl();

protected:
  virtual ErrorCode onQueryProcessList(Session &session,
                                       ProcessInfoMatch const &match,
                                       bool first, ProcessInfo &info);
  virtual ErrorCode onQueryProcessInfo(Session &session, ProcessId pid,
                                       ProcessInfo &info);

  virtual ErrorCode onExecuteProgram(Session &session,
                                     std::string const &command,
                                     uint32_t timeout,
                                     std::string const &workingDirectory,
                                     ProgramResult &result);

  virtual ErrorCode onFileOpen(Session &session, std::string const &path,
                               uint32_t flags, uint32_t mode, int &fd);
  virtual ErrorCode onFileClose(Session &session, int fd);

  virtual ErrorCode onFileExists(Session &session, std::string const &path);

protected:
  virtual ErrorCode onQueryUserName(Session &session, UserId const &uid,
                                    std::string &name);
  virtual ErrorCode onQueryGroupName(Session &session, GroupId const &gid,
                                     std::string &name);
  virtual ErrorCode onQueryWorkingDirectory(Session &session,
                                            std::string &workingDir);

protected:
  virtual ErrorCode onLaunchDebugServer(Session &session,
                                        std::string const &host, uint16_t &port,
                                        ProcessId &pid);

protected:
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
  virtual ErrorCode onQueryLaunchSuccess(Session &session, ProcessId pid);

private:
  void updateProcesses(ProcessInfoMatch const &match);
};
}
}

#endif // !__DebugServer2_GDBRemote_PlatformSessionImpl_h
