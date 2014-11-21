//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Base_h
#define __DebugServer2_Base_h

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
typedef SSIZE_T ssize_t;
#else
#include <cstdlib>
#endif

#if defined(__alpha__) || defined(_M_ALPHA)
#define ARCH_ALPHA
#elif defined(__arm__) || defined(_M_ARM)
#define ARCH_ARM
#elif defined(__aarch64__) || defined(_M_ARM64)
#define ARCH_ARM64
#elif defined(__hppa__)
#define ARCH_HPPA
#elif defined(__hppa64__)
#define ARCH_HPPA64
#elif defined(__i386__) || defined(_M_IX86)
#define ARCH_X86
#elif defined(__i860__)
#define ARCH_I860
#elif defined(__m68k__) || defined(_M_M68K)
#define ARCH_M68K
#elif defined(__m88k__)
#define ARCH_M88K
#elif defined(__mips__) || defined(_M_MRX000)
#define ARCH_MIPS
#elif defined(__mips64__)
#define ARCH_MIPS64
#elif defined(__ppc__)
#define ARCH_PPC
#elif defined(__ppc64__) || defined(_M_PPC)
#define ARCH_PPC64
#elif defined(__sparc__)
#define ARCH_SPARC
#elif defined(__sparc64__)
#define ARCH_SPARC64
#elif defined(__vax__)
#define ARCH_VAX
#elif defined(__x86_64__) || defined(_M_X64)
#define ARCH_X86_64
#else
#error "Architecture not supported."
#endif

template <typename TYPE, size_t SIZE>
static inline size_t array_size(TYPE const (&)[SIZE]) {
  return SIZE;
}

#endif // !__DebugServer2_Base_h
