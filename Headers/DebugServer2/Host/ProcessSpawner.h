//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Host_ProcessSpawner_h
#define __DebugServer2_Host_ProcessSpawner_h

#include "DebugServer2/Host/Base.h"

#if defined(_WIN32)
#include "DebugServer2/Host/Windows/ProcessSpawner.h"
#else
#include "DebugServer2/Host/POSIX/ProcessSpawner.h"
#endif

namespace ds2 {
namespace Host {

class ProcessSpawnerDelegate {
public:
  virtual void onProcessOutput(ProcessSpawner *ps, int stdfd,
                               void const *buffer, size_t length) = 0;
  virtual void onProcessInput(ProcessSpawner *ps, int stdfd, void *buffer,
                              size_t length, size_t &nread) = 0;
};
}
}

#endif // !__DebugServer2_Host_ProcessSpawner_h
