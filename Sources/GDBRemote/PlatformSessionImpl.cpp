//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

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

PlatformSessionImplBase::PlatformSessionImplBase()
    : DummySessionDelegateImpl() {}

ErrorCode PlatformSessionImplBase::onQueryProcessList(
    Session &session, ProcessInfoMatch const &match, bool first,
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

ErrorCode PlatformSessionImplBase::onQueryProcessInfo(Session &, ProcessId pid,
                                                      ProcessInfo &info) const {
  if (!Platform::GetProcessInfo(pid, info))
    return kErrorProcessNotFound;
  else
    return kSuccess;
}

ErrorCode PlatformSessionImplBase::onExecuteProgram(
    Session &, std::string const &command, uint32_t timeout,
    std::string const &workingDirectory, ProgramResult &result) {
  DS2LOG(Debug, "command='%s' timeout=%u", command.c_str(), timeout);

  ProcessSpawner ps;

  ps.setShellCommand(command);

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

ErrorCode PlatformSessionImplBase::onQueryUserName(Session &, UserId const &uid,
                                                   std::string &name) const {
  if (!Platform::GetUserName(uid, name))
    return kErrorNotFound;
  else
    return kSuccess;
}

ErrorCode PlatformSessionImplBase::onQueryGroupName(Session &,
                                                    GroupId const &gid,
                                                    std::string &name) const {
  if (!Platform::GetGroupName(gid, name))
    return kErrorNotFound;
  else
    return kSuccess;
}

ErrorCode PlatformSessionImplBase::onLaunchDebugServer(Session &session,
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
  std::string const &outputFilename = GetLogOutputFilename();
  if (outputFilename.length() > 0) {
    args.push_back("--log-file");
    args.push_back(outputFilename);
  }
#if defined(OS_POSIX)
  args.push_back("--setsid");
#endif
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

void PlatformSessionImplBase::updateProcesses(
    ProcessInfoMatch const &match) const {
  // TODO(fjricci) we should only add processes that match "match"
  Platform::EnumerateProcesses(
      true, UserId(), [&](ds2::ProcessInfo const &info) {
        _processIterationState.vals.push_back(info.pid);
      });

  _processIterationState.it = _processIterationState.vals.begin();
}
} // namespace GDBRemote
} // namespace ds2
