//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/Thread.h"

namespace ds2 {
namespace Target {
namespace Darwin {

ErrorCode Thread::prepareSoftwareSingleStep(Address const &address) {
  return kSuccess;
}
}
}
}
