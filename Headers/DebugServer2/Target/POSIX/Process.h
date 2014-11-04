//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Target_POSIX_Process_h
#define __DebugServer2_Target_POSIX_Process_h

#include "DebugServer2/Target/ProcessBase.h"
#include "DebugServer2/Host/POSIX/PTrace.h"

namespace ds2 {
namespace Target {
namespace POSIX {

class Process : public ds2::Target::ProcessBase {
public:
  enum { kFlagNewProcess = 0, kFlagAttachedProcess = (1 << 0) };

protected:
  std::set<int> _passthruSignals;

protected:
  Process();

public:
  virtual ~Process();

public:
  virtual ErrorCode detach();
  virtual ErrorCode interrupt();
  virtual ErrorCode terminate();
  virtual bool isAlive() const;

public:
  virtual ErrorCode suspend();
  virtual ErrorCode
  resume(int signal = 0,
         std::set<Thread *> const &excluded = std::set<Thread *>());

public:
  ErrorCode readMemory(Address const &address, void *data, size_t length,
                       size_t *nread = nullptr);
  ErrorCode writeMemory(Address const &address, void const *data, size_t length,
                        size_t *nwritten = nullptr);

public:
  void resetSignalPass();
  void setSignalPass(int signo, bool set);

public:
  virtual ErrorCode wait(int *status = nullptr, bool hang = true);

public:
  virtual Host::POSIX::PTrace &ptrace() const = 0;

public:
  static ds2::Target::Process *Create(int argc, char **argv);
  static ds2::Target::Process *
  Create(std::string const &path,
         StringCollection const &args = StringCollection());
  static ds2::Target::Process *Attach(ProcessId pid);
};
}
}
}

#endif // !__DebugServer2_Target_POSIX_Process_h
