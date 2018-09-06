//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/ProcessSpawner.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Host/Windows/ExtraWrappers.h"
#include "DebugServer2/Utils/Log.h"
#include "DebugServer2/Utils/String.h"

#include <windows.h>

using ds2::Host::Platform;

namespace ds2 {
namespace Host {

ProcessSpawner::ProcessSpawner() : _pid(0), _handle(0) {}

ProcessSpawner::~ProcessSpawner() {}

bool ProcessSpawner::setExecutable(std::string const &path) {
  if (_pid != 0)
    return false;

  _executablePath = path;
  return true;
}

bool ProcessSpawner::setShellCommand(std::string const &command) {
  if (_pid != 0) {
    return false;
  }

  setExecutable(command);
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

bool ProcessSpawner::addEnvironment(std::string const &key,
                                    std::string const &val) {
  if (_pid != 0)
    return false;

  _environment[key] = val;
  return true;
}

bool ProcessSpawner::setWorkingDirectory(std::string const &path) {
  if (_pid != 0)
    return false;

  _workingDirectory = path;
  return true;
}

ErrorCode ProcessSpawner::run(std::function<bool()> preExecAction) {
  std::vector<WCHAR> commandLine;
  std::vector<WCHAR> environment;
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;

  commandLine.push_back(L'"');
  std::wstring wideExecutablePath =
      ds2::Utils::NarrowToWideString(_executablePath);
  commandLine.insert(commandLine.end(), wideExecutablePath.begin(),
                     wideExecutablePath.end());
  commandLine.push_back(L'"');
  for (auto const &arg : _arguments) {
    commandLine.push_back(L' ');
    commandLine.push_back(L'"');
    std::wstring wideArg = ds2::Utils::NarrowToWideString(arg);
    for (auto const &ch : wideArg) {
      if (ch == L'"')
        commandLine.push_back(L'\\');
      commandLine.push_back(ch);
    }
    commandLine.push_back(L'"');
  }
  commandLine.push_back(L'\0');

  for (auto const &env : _environment) {
    std::wstring wideKey = ds2::Utils::NarrowToWideString(env.first);
    std::wstring wideValue = ds2::Utils::NarrowToWideString(env.second);
    environment.insert(environment.end(), wideKey.begin(), wideKey.end());
    environment.push_back(L'=');
    environment.insert(environment.end(), wideValue.begin(), wideValue.end());
    environment.push_back(L'\0');
  }
  environment.push_back(L'\0');

  memset(&si, 0, sizeof si);
  si.cb = sizeof si;

  // Note(sas): Not sure if we want DEBUG_ONLY_THIS_PROCESS here. Will need to
  // check back later.
  std::wstring wideWorkingDirectory =
      ds2::Utils::NarrowToWideString(_workingDirectory);
  BOOL result = CreateProcessW(
      nullptr, commandLine.data(), nullptr, nullptr, false,
      DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS | CREATE_UNICODE_ENVIRONMENT,
      environment.data(),
      wideWorkingDirectory.empty() ? nullptr : wideWorkingDirectory.c_str(),
      &si, &pi);

  if (!result)
    return Platform::TranslateError();

  _pid = pi.dwProcessId;
  _handle = pi.hProcess;

  _tid = pi.dwThreadId;
  _threadHandle = pi.hThread;

  return kSuccess;
}
} // namespace Host
} // namespace ds2
