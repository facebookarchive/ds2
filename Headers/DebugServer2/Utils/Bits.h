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

#include "DebugServer2/Utils/CompilerSupport.h"

#if defined(OS_WIN32)
#include <intrin.h>
#endif
#include <type_traits>

namespace ds2 {
namespace Utils {

// We apply the builtin bit functions on chunks of sizeof(int) * 8 bits.
// However, we can't do full shifts of this size because only the least
// signifitive bits are used and clang complains, so we shift twice half
// the size.
static const size_t kHalfIntBits = sizeof(unsigned int) * 4;

template <typename T>
static typename std::make_unsigned<T>::type MakeUnsigned(T number) {
  return static_cast<typename std::make_unsigned<T>::type>(number);
}

static inline unsigned int FFS(unsigned int number) {
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
  return __builtin_ffs(number);
#elif defined(COMPILER_MSVC)
  unsigned long index;
  if (_BitScanForward(&index, number)) {
    return index + 1;
  }
  return 0;
#else
  for (int i = 1; number != 0; i++, number >>= 1) {
    if (number & 1) {
      return i;
    }
  }
  return 0;
#endif
}

static inline unsigned int PopCount(unsigned int number) {
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
  return __builtin_popcount(number);
#elif defined(COMPILER_MSVC) && defined(ARCH_ARM)
  return _CountOneBits(number);
#else
  unsigned int count = 0;
  while (number != 0) {
    count++;
    number = number & (number - 1);
  }
  return count;
#endif
}

template <typename T> static inline unsigned int FFS(T number) {
  auto uNumber = MakeUnsigned(number);
  unsigned int ffs = 0;
  while (uNumber > 0) {
    ffs += FFS(static_cast<unsigned int>(uNumber));
    if (ffs != 0) {
      return ffs;
    }
    uNumber >>= kHalfIntBits;
    uNumber >>= kHalfIntBits;
    ffs += 2 * kHalfIntBits;
  }
  return 0;
}

template <typename T> static inline unsigned int PopCount(T number) {
  auto uNumber = MakeUnsigned(number);
  unsigned int count = 0;
  while (uNumber > 0) {
    count += PopCount(static_cast<unsigned int>(number));
    uNumber >>= kHalfIntBits;
    uNumber >>= kHalfIntBits;
  }
  return count;
}

template <typename T> static inline void EnableBit(T &number, int idx) {
  number |= static_cast<typename std::make_unsigned<T>::type>(1) << idx;
}

template <typename T> static inline void DisableBit(T &number, int idx) {
  number &= ~(static_cast<typename std::make_unsigned<T>::type>(1) << idx);
}

// Zeros bits from startIdx through (endIdx - 1) in number
template <typename T>
static inline void DisableBits(T &number, int startIdx, int endIdx) {
  auto mask = (static_cast<typename std::make_unsigned<T>::type>(1)
               << (endIdx - startIdx)) -
              1;
  number &= ~(mask << startIdx);
}

template <typename T>
static inline void Align(T &number, unsigned int alignment) {
  unsigned int clear = alignment - 1;
  number = (number + clear) & ~clear;
}
} // namespace Utils
} // namespace ds2
