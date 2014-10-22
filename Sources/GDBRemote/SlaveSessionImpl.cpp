//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#define __DS2_LOG_CLASS_NAME__ "SlaveSession"

#include "DebugServer2/GDBRemote/SlaveSessionImpl.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Log.h"

using ds2::GDBRemote::SlaveSessionImpl;
using ds2::ErrorCode;

SlaveSessionImpl::SlaveSessionImpl() : DebugSessionImpl() {}

ErrorCode SlaveSessionImpl::onAttach(Session &session, ProcessId pid,
                                     AttachMode mode, StopCode &stop) {
  if (_process != nullptr)
    return kErrorAlreadyExist;

  if (mode != kAttachNow)
    return kErrorInvalidArgument;

  DS2LOG(SlaveSession, Info, "attaching to pid %u", pid);
  _process = Target::Process::Attach(pid);
  DS2LOG(SlaveSession, Debug, "_process=%p", _process);
  if (_process == nullptr)
    return kErrorProcessNotFound;

  ErrorCode error = _process->wait();
  DS2LOG(SlaveSession, Debug, "wait=%d [%s]", error, GetErrorCodeString(error));
  if (error != kSuccess) {
    delete _process;
    _process = nullptr;
    return error;
  }

  return queryStopCode(session, pid, stop);
}

ErrorCode SlaveSessionImpl::onAttach(Session &, std::string const &name,
                                     AttachMode mode, StopCode &stop) {
  return kErrorProcessNotFound;
}
