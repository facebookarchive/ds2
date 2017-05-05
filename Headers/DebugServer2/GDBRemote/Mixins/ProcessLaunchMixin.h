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
#include "DebugServer2/Host/File.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Host/ProcessSpawner.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Target/Thread.h"

#include <utility>

using ds2::Host::Platform;

namespace ds2 {
namespace GDBRemote {

template <typename T> class ProcessLaunchMixin : public T {
protected:
  typedef std::map<std::string, std::string> EnvironmentMap;

protected:
  bool _disableASLR;
  std::string _workingDirectory;
  EnvironmentMap _environment;
  std::string _stdFile[3];
  StringCollection _arguments;

public:
  template <typename... Args>
  explicit ProcessLaunchMixin(Args &&... args)
      : T(std::forward<Args>(args)...), _disableASLR(false),
        _workingDirectory(Platform::GetWorkingDirectory()) {}

public:
  ErrorCode onDisableASLR(Session &session, bool disable) override;
  ErrorCode onSetArchitecture(Session &session,
                              std::string const &architecture) override;
  ErrorCode onSetWorkingDirectory(Session &session,
                                  std::string const &path) override;
  ErrorCode onQueryWorkingDirectory(Session &session,
                                    std::string &workingDir) const override;
  ErrorCode onSetEnvironmentVariable(Session &session, std::string const &name,
                                     std::string const &value) override;
  ErrorCode onSetStdFile(Session &session, int fileno,
                         std::string const &path) override;
  ErrorCode onSetProgramArguments(Session &session,
                                  StringCollection const &args) override;
  ErrorCode onQueryLaunchSuccess(Session &session,
                                 ProcessId pid) const override;
};
} // namespace GDBRemote
} // namespace ds2

#include "../Sources/GDBRemote/Mixins/ProcessLaunchMixin.hpp"
