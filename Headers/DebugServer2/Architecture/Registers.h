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

#if defined(__arm__)
#include "DebugServer2/Architecture/ARM/Registers.h"
#elif defined(__aarch64__)
#include "DebugServer2/Architecture/ARM/Registers.h"
#include "DebugServer2/Architecture/ARM64/Registers.h"
#elif defined(__i386__)
#include "DebugServer2/Architecture/I386/Registers.h"
#elif defined(__x86_64__)
#include "DebugServer2/Architecture/I386/Registers.h"
#include "DebugServer2/Architecture/X86_64/Registers.h"
#elif defined(__mips__)
#include "DebugServer2/Architecture/MIPS/Registers.h"
#elif defined(__mips64__)
#include "DebugServer2/Architecture/MIPS/Registers.h"
#include "DebugServer2/Architecture/MIPS64/Registers.h"
#elif defined(__powerpc__)
#include "DebugServer2/Architecture/PowerPC/Registers.h"
#elif defined(__powerpc64__)
#include "DebugServer2/Architecture/PowerPC/Registers.h"
#include "DebugServer2/Architecture/PowerPC64/Registers.h"
#elif defined(__sparc__)
#include "DebugServer2/Architecture/SPARC/Registers.h"
#elif defined(__sparc64__)
#include "DebugServer2/Architecture/SPARC/Registers.h"
#include "DebugServer2/Architecture/SPARC64/Registers.h"
#else
#error "Unknown target architecture."
#endif

#endif // !__DebugServer2_Architecture_Registers_h
