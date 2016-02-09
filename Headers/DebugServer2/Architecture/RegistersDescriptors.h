//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Architecture_RegistersDescriptors_h
#define __DebugServer2_Architecture_RegistersDescriptors_h

#include "DebugServer2/Base.h"

#define USE_DESCRIPTORS(ARCH)                                                  \
  namespace ds2 {                                                              \
  namespace Architecture {                                                     \
  using ds2::Architecture::ARCH::GDB;                                          \
  using ds2::Architecture::ARCH::LLDB;                                         \
  }                                                                            \
  }

#if defined(ARCH_ARM)
#include "DebugServer2/Architecture/ARM/RegistersDescriptors.h"
USE_DESCRIPTORS(ARM);
#elif defined(ARCH_ARM64)
#include "DebugServer2/Architecture/ARM64/RegistersDescriptors.h"
USE_DESCRIPTORS(ARM64);
#elif defined(ARCH_X86)
#include "DebugServer2/Architecture/X86/RegistersDescriptors.h"
USE_DESCRIPTORS(X86);
#elif defined(ARCH_X86_64)
#include "DebugServer2/Architecture/X86_64/RegistersDescriptors.h"
USE_DESCRIPTORS(X86_64);
#else
#error "Architecture not supported."
#endif

#undef USE_DESCRIPTORS

#endif // !__DebugServer2_Architecture_RegistersDescriptors_h
