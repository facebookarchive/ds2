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

#include "DebugServer2/GDBRemote/Mixins/ProcessLaunchMixin.h"

namespace ds2 {
namespace GDBRemote {

template <typename T>
ErrorCode ProcessLaunchMixin<T>::onDisableASLR(Session &session, bool disable) {
  _disableASLR = disable;
  return kSuccess;
}

template <typename T>
ErrorCode
ProcessLaunchMixin<T>::onSetArchitecture(Session &,
                                         std::string const &architecture) {
  // Ignored for now.
  return kSuccess;
}

template <typename T>
ErrorCode
ProcessLaunchMixin<T>::onSetWorkingDirectory(Session &,
                                             std::string const &path) {
  _workingDirectory = path;
  return kSuccess;
}

template <typename T>
ErrorCode
ProcessLaunchMixin<T>::onQueryWorkingDirectory(Session &,
                                               std::string &workingDir) const {
  workingDir = _workingDirectory;
  return kSuccess;
}

template <typename T>
ErrorCode ProcessLaunchMixin<T>::onSetEnvironmentVariable(
    Session &, std::string const &name, std::string const &value) {
  if (value.empty()) {
    _environment.erase(name);
  } else {
    _environment.insert(std::make_pair(name, value));
  }
  return kSuccess;
}

template <typename T>
ErrorCode ProcessLaunchMixin<T>::onSetStdFile(Session &, int fileno,
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

template <typename T>
ErrorCode
ProcessLaunchMixin<T>::onSetProgramArguments(Session &,
                                             StringCollection const &args) {
  _arguments = args;
  for (auto const &arg : _arguments) {
    DS2LOG(Debug, "arg=%s", arg.c_str());
  }
  return kSuccess;
}

template <typename T>
ErrorCode ProcessLaunchMixin<T>::onQueryLaunchSuccess(Session &,
                                                      ProcessId) const {
  return kSuccess;
}
} // namespace GDBRemote
} // namespace ds2
