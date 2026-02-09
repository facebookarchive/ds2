// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#pragma once

#include "DebugServer2/GDBRemote/DebugSessionImpl.h"

namespace ds2 {
namespace GDBRemote {

class SlaveSessionImpl : public DebugSessionImpl {
public:
  SlaveSessionImpl();
};
} // namespace GDBRemote
} // namespace ds2
