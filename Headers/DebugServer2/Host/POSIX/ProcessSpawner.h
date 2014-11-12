//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Host_POSIX_ProcessSpawner_h
#define __DebugServer2_Host_POSIX_ProcessSpawner_h

#include "DebugServer2/Types.h"

#include <thread>

namespace ds2 {
namespace Host {

class ProcessSpawnerDelegate;

class ProcessSpawner {
protected:
  enum RedirectMode {
    kRedirectConsole,
    kRedirectNull,
    kRedirectFile,
    kRedirectBuffer,
    kRedirectDelegate
  };

protected:
  struct RedirectDescriptor {
    RedirectMode mode;
    ProcessSpawnerDelegate *delegate;
    std::string path;
    int fd;

    RedirectDescriptor() : mode(kRedirectConsole), delegate(nullptr), fd(-1) {}
  };

protected:
  std::string _executablePath;
  StringCollection _arguments;
  std::string _workingDirectory;
  ProcessSpawnerDelegate *_delegate;
  std::thread _delegateThread;
  RedirectDescriptor _descriptors[3];
  std::string _outputBuffer;
  int _exitStatus;
  int _signalCode;
  ProcessId _pid;

public:
  ProcessSpawner();

public:
  bool setExecutable(std::string const &path);
  bool setWorkingDirectory(std::string const &path);

public:
  bool setArguments(StringCollection const &args);

  template <typename... Args> inline bool setArguments(Args const &... args) {
    std::string args_[] = {args...};
    return setArguments(StringCollection(&args_[0], &args_[sizeof...(Args)]));
  }

public:
  bool redirectInputToConsole();
  bool redirectOutputToConsole();
  bool redirectErrorToConsole();

public:
  bool redirectInputToNull();
  bool redirectOutputToNull();
  bool redirectErrorToNull();

public:
  bool redirectInputToFile(std::string const &path);
  bool redirectOutputToFile(std::string const &path);
  bool redirectErrorToFile(std::string const &path);

public:
  bool redirectOutputToBuffer();
  bool redirectErrorToBuffer();

public:
  bool redirectInputToDelegate(ProcessSpawnerDelegate *delegate);
  bool redirectOutputToDelegate(ProcessSpawnerDelegate *delegate);
  bool redirectErrorToDelegate(ProcessSpawnerDelegate *delegate);

public:
  ErrorCode run();
  ErrorCode wait();
  bool isRunning() const;

public:
  inline ProcessId pid() const { return _pid; }
  inline int exitStatus() const { return _exitStatus; }
  inline int signalCode() const { return _signalCode; }

public:
  inline std::string const &output() const { return _outputBuffer; }

private:
  void redirectionThread();
};
}
}

#endif // !__DebugServer2_Host_POSIX_ProcessSpawner_h
