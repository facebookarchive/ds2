//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#define __DS2_LOG_CLASS_NAME__ "ProcessSpawner"

#include "DebugServer2/Host/ProcessSpawner.h"
#include "DebugServer2/Log.h"

#include <windows.h>

using ds2::Host::ProcessSpawner;
using ds2::ErrorCode;

ProcessSpawner::ProcessSpawner() : _processHandle(0), _pid(0) {}

ProcessSpawner::~ProcessSpawner() {}

bool ProcessSpawner::setExecutable(std::string const &path) {
  if (_pid != 0)
    return false;

  _executablePath = path;
  return true;
}

bool ProcessSpawner::setArguments(StringCollection const &args) {
  if (_pid != 0)
    return false;

  _arguments = args;
  return true;
}

bool ProcessSpawner::setEnvironment(EnvironmentBlock const &env) {
  if (_pid != 0)
    return false;

  _environment = env;
  return true;
}

bool ProcessSpawner::setWorkingDirectory(std::string const &path) {
  if (_pid != 0)
    return false;

  _workingDirectory = path;
  return true;
}

ErrorCode ProcessSpawner::run(std::function<bool()> preExecAction) {
  std::vector<char> commandLine;
  std::vector<char> environment;
  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  commandLine.push_back('"');
  commandLine.insert(commandLine.end(), _executablePath.begin(),
                     _executablePath.end());
  commandLine.push_back('"');
  for (auto const &arg : _arguments) {
    commandLine.push_back(' ');
    commandLine.push_back('"');
    for (auto const &ch : arg) {
      if (ch == '"')
        commandLine.push_back('\\');
      commandLine.push_back(ch);
    }
    commandLine.push_back('"');
  }
  commandLine.push_back('\0');

  for (auto const &env : _environment) {
    environment.insert(environment.end(), env.first.begin(), env.first.end());
    environment.push_back('=');
    environment.insert(environment.end(), env.second.begin(), env.second.end());
    environment.push_back('\0');
  }
  environment.push_back('\0');

  memset(&si, 0, sizeof si);
  si.cb = sizeof si;

  // Note(sas): Not sure if we want DEBUG_ONLY_THIS_PROCESS here. Will need to
  // check back later.
  BOOL result = CreateProcess(
      nullptr, commandLine.data(), nullptr, nullptr, false,
      DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS, environment.data(),
      _workingDirectory.empty() ? nullptr : _workingDirectory.c_str(), &si,
      &pi);

  _processHandle = pi.hProcess;
  _pid = pi.dwProcessId;

  return result ? kSuccess : kErrorUnknown;
}
