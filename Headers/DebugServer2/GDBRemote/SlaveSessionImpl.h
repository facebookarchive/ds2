//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_GDBRemote_SlaveSessionImpl_h
#define __DebugServer2_GDBRemote_SlaveSessionImpl_h

#include "DebugServer2/GDBRemote/DebugSessionImpl.h"

namespace ds2 {
namespace GDBRemote {

class SlaveSessionImpl : public DebugSessionImpl {
public:
  SlaveSessionImpl();

protected:
  virtual ErrorCode onAttach(Session &session, ProcessId pid, AttachMode mode,
                             StopCode &stop);
  virtual ErrorCode onAttach(Session &session, std::string const &name,
                             AttachMode mode, StopCode &stop);
};
}
}

#endif // !__DebugServer2_GDBRemote_SlaveSessionImpl_h
