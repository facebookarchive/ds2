// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#pragma once

#include "DebugServer2/Base.h"

#if defined(ARCH_ARM)
#include "DebugServer2/Architecture/ARM/Registers.h"
#elif defined(ARCH_ARM64)
#include "DebugServer2/Architecture/ARM/Registers.h"
#include "DebugServer2/Architecture/ARM64/Registers.h"
#elif defined(ARCH_X86)
#include "DebugServer2/Architecture/I386/Registers.h"
#elif defined(ARCH_X86_64)
#include "DebugServer2/Architecture/I386/Registers.h"
#include "DebugServer2/Architecture/X86_64/Registers.h"
#else
#error "Architecture not supported."
#endif
