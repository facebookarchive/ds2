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

#include "DebugServer2/Types.h"

#include <functional>

namespace ds2 {
namespace Host {

class ProcessSpawner {
protected:
  std::string _executablePath;
  StringCollection _arguments;
  EnvironmentBlock _environment;
  std::string _workingDirectory;
  ProcessId _pid;
  HANDLE _handle;
  ThreadId _tid;
  HANDLE _threadHandle;

public:
  ProcessSpawner();
  ~ProcessSpawner();

protected:
  typedef std::function<void(void *buf, size_t size)> RedirectDelegate;

public:
  bool setExecutable(std::string const &path);
  bool setShellCommand(std::string const &command);
  bool setWorkingDirectory(std::string const &path);

public:
  bool setArguments(StringCollection const &args);

  template <typename... Args> inline bool setArguments(Args const &... args) {
    std::string args_[] = {args...};
    return setArguments(StringCollection(&args_[0], &args_[sizeof...(Args)]));
  }

public:
  bool setEnvironment(EnvironmentBlock const &args);
  bool addEnvironment(std::string const &key, std::string const &val);

public:
  bool redirectInputToConsole() { return false; }
  bool redirectOutputToConsole() { return false; }
  bool redirectErrorToConsole() { return false; }

public:
  bool redirectInputToNull() { return false; }
  bool redirectOutputToNull() { return false; }
  bool redirectErrorToNull() { return false; }

public:
  bool redirectInputToFile(std::string const &path) { return false; }
  bool redirectOutputToFile(std::string const &path) { return false; }
  bool redirectErrorToFile(std::string const &path) { return false; }

public:
  bool redirectOutputToBuffer() { return false; }
  bool redirectErrorToBuffer() { return false; }

public:
  bool redirectInputToTerminal() { return false; }

public:
  bool redirectOutputToDelegate(RedirectDelegate delegate) { return false; }
  bool redirectErrorToDelegate(RedirectDelegate delegate) { return false; }

public:
  ErrorCode run(std::function<bool()> preExecAction = []() { return true; });
  ErrorCode wait() { return kErrorUnsupported; }
  bool isRunning() const { return false; }
  void flushAndExit() {}

public:
  inline ProcessId pid() const { return _pid; }
  inline HANDLE handle() const { return _handle; }
  inline ThreadId tid() const { return _tid; }
  inline HANDLE threadHandle() const { return _threadHandle; }
  inline int exitStatus() const { return 0; }
  inline int signalCode() const { return 0; }

public:
  ErrorCode input(ByteVector const &buf) { return kErrorUnsupported; }

  inline std::string const &output() const {
    static std::string s;
    return s;
  }

private:
  void redirectionThread();
};
} // namespace Host
} // namespace ds2
