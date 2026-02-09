// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#pragma once

#include "DebugServer2/GDBRemote/DummySessionDelegateImpl.h"
#include "DebugServer2/GDBRemote/Mixins/FileOperationsMixin.h"
#include "DebugServer2/GDBRemote/Mixins/ProcessLaunchMixin.h"
#include "DebugServer2/Utils/MPL.h"

namespace ds2 {
namespace GDBRemote {

class PlatformSessionImplBase : public DummySessionDelegateImpl {
protected:
  // a struct to help iterate over the process list for onQueryProcessList
  mutable IterationState<ProcessId> _processIterationState;

public:
  PlatformSessionImplBase();

protected:
  ErrorCode onQueryProcessList(Session &session, ProcessInfoMatch const &match,
                               bool first, ProcessInfo &info) const override;
  ErrorCode onQueryProcessInfo(Session &session, ProcessId pid,
                               ProcessInfo &info) const override;

  ErrorCode onExecuteProgram(Session &session, std::string const &command,
                             uint32_t timeout,
                             std::string const &workingDirectory,
                             ProgramResult &result) override;

protected:
  ErrorCode onQueryUserName(Session &session, UserId const &uid,
                            std::string &name) const override;
  ErrorCode onQueryGroupName(Session &session, GroupId const &gid,
                             std::string &name) const override;

protected:
  ErrorCode onLaunchDebugServer(Session &session, std::string const &host,
                                uint16_t &port, ProcessId &pid) override;

private:
  void updateProcesses(ProcessInfoMatch const &match) const;
};

using PlatformSessionImpl =
    Utils::MixinApply<PlatformSessionImplBase, FileOperationsMixin,
                      ProcessLaunchMixin>;
} // namespace GDBRemote
} // namespace ds2
