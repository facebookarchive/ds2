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
#include "DebugServer2/Utils/Log.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace ds2 {

static inline char NibbleToHex(uint8_t byte) {
  return "0123456789abcdef"[byte & 0x0f];
}

static inline uint8_t HexToNibble(char ch) {
  if (ch >= '0' && ch <= '9')
    return ch - '0';
  else if (ch >= 'a' && ch <= 'f')
    return ch - 'a' + 10;
  else if (ch >= 'A' && ch <= 'F')
    return ch - 'A' + 10;
  DS2_UNREACHABLE();
}

static inline uint8_t HexToByte(char const *chars) {
  return (HexToNibble(chars[0]) << 4) | HexToNibble(chars[1]);
}

template <typename T> static inline std::string ToHex(T const &vec) {
  std::string result;
  for (char n : vec) {
    result += NibbleToHex(n >> 4);
    result += NibbleToHex(n & 0x0f);
  }
  return result;
}

static inline ByteVector HexToByteVector(std::string const &str) {
  ByteVector result;
  DS2ASSERT(str.size() % 2 == 0);
  for (size_t n = 0; n < str.size(); n += 2) {
    result.emplace_back(HexToByte(&str[n]));
  }
  return result;
}

static inline std::string HexToString(std::string const &str) {
  std::string result;
  DS2ASSERT(str.size() % 2 == 0);
  for (size_t n = 0; n < str.size(); n += 2) {
    result += static_cast<char>(HexToByte(&str[n]));
  }
  return result;
}
} // namespace ds2
