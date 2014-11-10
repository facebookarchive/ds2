//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Architecture_Registers_h
#define __DebugServer2_Architecture_Registers_h

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
#elif defined(ARCH_MIPS)
#include "DebugServer2/Architecture/MIPS/Registers.h"
#elif defined(ARCH_MIPS64)
#include "DebugServer2/Architecture/MIPS/Registers.h"
#include "DebugServer2/Architecture/MIPS64/Registers.h"
#elif defined(ARCH_PPC)
#include "DebugServer2/Architecture/PowerPC/Registers.h"
#elif defined(ARCH_PPC64)
#include "DebugServer2/Architecture/PowerPC/Registers.h"
#include "DebugServer2/Architecture/PowerPC64/Registers.h"
#elif defined(ARCH_SPARC)
#include "DebugServer2/Architecture/SPARC/Registers.h"
#elif defined(ARCH_SPARC64)
#include "DebugServer2/Architecture/SPARC/Registers.h"
#include "DebugServer2/Architecture/SPARC64/Registers.h"
#else
#error "Unknown target architecture."
#endif

#endif // !__DebugServer2_Architecture_Registers_h
