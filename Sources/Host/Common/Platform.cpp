//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Base.h"

namespace ds2 {
namespace Host {

ds2::CPUType Platform::GetCPUType() {
#if defined(ARCH_ARM) || defined(ARCH_ARM64)
  return (sizeof(void *) == 8) ? kCPUTypeARM64 : kCPUTypeARM;
#elif defined(ARCH_X86) || defined(ARCH_X86_64)
  return (sizeof(void *) == 8) ? kCPUTypeX86_64 : kCPUTypeI386;
#else
#error "Architecture not supported."
#endif
}

ds2::CPUSubType Platform::GetCPUSubType() {
#if defined(ARCH_ARM)
#if defined(__ARM_ARCH_7EM__)
  return kCPUSubTypeARM_V7EM;
#elif defined(__ARM_ARCH_7M__)
  return kCPUSubTypeARM_V7M;
#elif defined(OS_WIN32) || defined(__ARM_ARCH_7__) ||                          \
    defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__)
  return kCPUSubTypeARM_V7;
#endif
#endif // ARCH_ARM
  return kCPUSubTypeInvalid;
}

ds2::Endian Platform::GetEndian() {
#if defined(ENDIAN_LITTLE)
  return kEndianLittle;
#elif defined(ENDIAN_BIG)
  return kEndianBig;
#elif defined(ENDIAN_MIDDLE)
  return kEndianPDP;
#else
  return kEndianUnknown;
#endif
}

size_t Platform::GetPointerSize() { return sizeof(void *); }
} // namespace Host
} // namespace ds2
