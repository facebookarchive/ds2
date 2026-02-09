// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#pragma once

#include "DebugServer2/Base.h"

#if defined(OS_POSIX)
#include "DebugServer2/Host/POSIX/ProcessSpawner.h"
#elif defined(OS_WIN32)
#include "DebugServer2/Host/Windows/ProcessSpawner.h"
#else
#error "Target not supported."
#endif
