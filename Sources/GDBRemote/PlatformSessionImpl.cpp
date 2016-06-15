//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#define __DS2_LOG_CLASS_NAME__ "PlatformSession"

#include "DebugServer2/GDBRemote/PlatformSessionImpl.h"
#include "DebugServer2/GDBRemote/Session.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Host/ProcessSpawner.h"
#include "DebugServer2/Utils/Log.h"

#include <sstream>

using ds2::Host::Platform;
using ds2::Host::ProcessSpawner;

namespace ds2 {
namespace GDBRemote {

PlatformSessionImpl::PlatformSessionImpl()
    : DummySessionDelegateImpl(), _disableASLR(false),
      _workingDirectory(Platform::GetWorkingDirectory()) {}

ErrorCode PlatformSessionImpl::onQueryProcessList(Session &session,
                                                  ProcessInfoMatch const &match,
                                                  bool first,
                                                  ProcessInfo &info) const {
  if (first) {
    updateProcesses(match);
  }

  if (_processIterationState.it == _processIterationState.vals.end())
    return kErrorProcessNotFound;

  if (!Platform::GetProcessInfo(*_processIterationState.it++, info))
    return kErrorProcessNotFound;

  return kSuccess;
}

ErrorCode PlatformSessionImpl::onQueryProcessInfo(Session &, ProcessId pid,
                                                  ProcessInfo &info) const {
  if (!Platform::GetProcessInfo(pid, info))
    return kErrorProcessNotFound;
  else
    return kSuccess;
}

ErrorCode PlatformSessionImpl::onExecuteProgram(
    Session &, std::string const &command, uint32_t timeout,
    std::string const &workingDirectory, ProgramResult &result) {
  DS2LOG(Debug, "command='%s' timeout=%u", command.c_str(), timeout);

  ProcessSpawner ps;

  ps.setExecutable(command);

  ps.redirectInputToNull();
  ps.redirectOutputToBuffer();
  ps.redirectErrorToBuffer();
  ps.setWorkingDirectory(workingDirectory);

  ErrorCode error;
  error = ps.run();
  if (error != kSuccess)
    return error;
  error = ps.wait();
  if (error != kSuccess)
    return error;

  result.status = ps.exitStatus();
  result.signal = ps.signalCode();
  result.output = ps.output();

  return kSuccess;
}

ErrorCode PlatformSessionImpl::onFileExists(Session &,
                                            std::string const &path) {
  if (!Platform::IsFilePresent(path))
    return kErrorNotFound;
  else
    return kSuccess;
}

ErrorCode PlatformSessionImpl::onQueryUserName(Session &, UserId const &uid,
                                               std::string &name) const {
  if (!Platform::GetUserName(uid, name))
    return kErrorNotFound;
  else
    return kSuccess;
}

ErrorCode PlatformSessionImpl::onQueryGroupName(Session &, GroupId const &gid,
                                                std::string &name) const {
  if (!Platform::GetGroupName(gid, name))
    return kErrorNotFound;
  else
    return kSuccess;
}

ErrorCode
PlatformSessionImpl::onQueryWorkingDirectory(Session &,
                                             std::string &workingDir) const {
  workingDir = _workingDirectory;
  return kSuccess;
}

ErrorCode PlatformSessionImpl::onLaunchDebugServer(Session &session,
                                                   std::string const &host,
                                                   uint16_t &port,
                                                   ProcessId &pid) {
  ProcessSpawner ps;
  StringCollection args;

  ps.setExecutable(Platform::GetSelfExecutablePath());
  args.push_back("slave");
  if (GetLogLevel() == kLogLevelDebug) {
    args.push_back("--debug");
  } else if (GetLogLevel() == kLogLevelPacket) {
    args.push_back("--remote-debug");
  }
  args.push_back("--setsid");
  ps.setArguments(args);
  ps.redirectInputToNull();
  ps.redirectOutputToBuffer();

  ErrorCode error;
  error = ps.run();
  if (error != kSuccess)
    return error;
  error = ps.wait();
  if (error != kSuccess)
    return error;

  if (ps.exitStatus() != 0)
    return kErrorInvalidArgument;

  std::istringstream ss;
  ss.str(ps.output());
  ss >> port >> pid;

  return kSuccess;
}

void PlatformSessionImpl::updateProcesses(ProcessInfoMatch const &match) const {
  // TODO(fjricci) we should only add processes that match "match"
  Platform::EnumerateProcesses(
      true, UserId(), [&](ds2::ProcessInfo const &info) {
        _processIterationState.vals.push_back(info.pid);
      });

  _processIterationState.it = _processIterationState.vals.begin();
}

ErrorCode PlatformSessionImpl::onDisableASLR(Session &, bool disable) {
  _disableASLR = disable;
  return kSuccess;
}

ErrorCode PlatformSessionImpl::onSetEnvironmentVariable(
    Session &, std::string const &name, std::string const &value) {
  if (value.empty()) {
    _environment.erase(name);
  } else {
    _environment.insert(std::make_pair(name, value));
  }
  return kSuccess;
}

ErrorCode PlatformSessionImpl::onSetWorkingDirectory(Session &,
                                                     std::string const &path) {
  _workingDirectory = path;
  return kSuccess;
}

ErrorCode PlatformSessionImpl::onSetStdFile(Session &, int fileno,
                                            std::string const &path) {
  DS2LOG(Debug, "stdfile[%d] = %s", fileno, path.c_str());

  if (fileno < 0 || fileno > 2)
    return kErrorInvalidArgument;

  if (fileno == 0) {
    //
    // TODO it seems that QSetSTDIN is the first message sent when
    // launching a new process, it may be sane to invalidate all
    // the process state at this point.
    //
    _disableASLR = false;
    _arguments.clear();
    _environment.clear();
    _workingDirectory.clear();
    _stdFile[0].clear();
    _stdFile[1].clear();
    _stdFile[2].clear();
  }

  _stdFile[fileno] = path;
  return kSuccess;
}

ErrorCode
PlatformSessionImpl::onSetArchitecture(Session &,
                                       std::string const &architecture) {
  // Ignored for now
  return kSuccess;
}

ErrorCode
PlatformSessionImpl::onSetProgramArguments(Session &,
                                           StringCollection const &args) {
  _arguments = args;
  for (auto const &arg : _arguments) {
    DS2LOG(Debug, "arg=%s", arg.c_str());
  }
  return kSuccess;
}

ErrorCode PlatformSessionImpl::onQueryLaunchSuccess(Session &,
                                                    ProcessId) const {
  return kSuccess;
}
}
}
