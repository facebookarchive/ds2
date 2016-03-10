//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Utils_Bits_h
#define __DebugServer2_Utils_Bits_h

#include <type_traits>

#include "DebugServer2/Utils/CompilerSupport.h"

namespace ds2 {
namespace Utils {

// We apply the builtin bit functions on chunks of sizeof(int) * 8 bits.
// However, we can't do full shifts of this size because only the least
// signifitive bits are used and clang complains, so we shift twice half
// the size.
static const size_t kHalfIntBits = sizeof(int) * 4;

template <typename T> static typename std::make_unsigned<T>::type MakeUnsigned(T number) {
  return static_cast<typename std::make_unsigned<T>::type>(number);
}

static inline unsigned int FFS(unsigned int number) {
#if __has_builtin(__builtin_ffs)
  return static_cast<unsigned int>(__builtin_ffs(static_cast<int>(number)));
#elif defined(_MSC_VER)
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
#if __has_builtin(__builtin_popcount)
  return static_cast<unsigned int>(__builtin_popcount(static_cast<int>(number)));
#elif defined(_MSC_VER)
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
  while (uNumber > 0) {
    unsigned int ffs = FFS(static_cast<typeof(uNumber)>(uNumber));
    if (ffs != 0) {
      return ffs;
    }
    uNumber >>= kHalfIntBits;
    uNumber >>= kHalfIntBits;
  }
  return 0;
}

template <typename T> static inline unsigned int PopCount(T number) {
  auto uNumber = MakeUnsigned(number);
  unsigned int count = 0;
  while (uNumber > 0) {
    count += PopCount(static_cast<typeof(uNumber)>(number));
    uNumber >>= kHalfIntBits;
    uNumber >>= kHalfIntBits;
  }
  return count;
}
}
}

#endif // !__DebugServer2_Utils_Bits_h
