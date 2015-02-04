//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Host_Windows_ProcessSpawner_h
#define __DebugServer2_Host_Windows_ProcessSpawner_h

#include <functional>

namespace ds2 {
namespace Host {

class ProcessSpawner {
protected:
  typedef std::function<void(void *buf, size_t size)> RedirectDelegate;

public:
  bool setExecutable(std::string const &path) { return false; }
  bool setWorkingDirectory(std::string const &path) { return false; }

public:
  bool setArguments(StringCollection const &args) { return false; }

  template <typename... Args> inline bool setArguments(Args const &... args) {
    static std::string const args_[] = {args...};
    return setArguments(StringCollection(&args_[0], &args_[sizeof...(Args)]));
  }

public:
  bool setEnvironment(StringCollection const &args) { return false; }

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
  bool redirectOutputToDelegate(RedirectDelegate delegate) { return false; }
  bool redirectErrorToDelegate(RedirectDelegate delegate) { return false; }

public:
  ErrorCode run() { return kErrorUnsupported; }
  ErrorCode wait() { return kErrorUnsupported; }
  bool isRunning() const { return false; }

public:
  inline ProcessId pid() const { return 0; }
  inline int exitStatus() const { return 0; }
  inline int signalCode() const { return 0; }

public:
  inline std::string const &output() const {
    static std::string s;
    return s;
  }

private:
  void redirectionThread();
};
}
}

#endif // !__DebugServer2_Host_Windows_ProcessSpawner_h
