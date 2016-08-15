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

#include "DebugServer2/Base.h"

#if defined(OS_POSIX)
#include "DebugServer2/Host/POSIX/ProcessSpawner.h"
#elif defined(OS_WIN32)
#include "DebugServer2/Host/Windows/ProcessSpawner.h"
#else
#error "Target not supported."
#endif
