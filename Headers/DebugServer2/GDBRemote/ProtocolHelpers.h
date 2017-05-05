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

#include "DebugServer2/Types.h"
#include "DebugServer2/Utils/Log.h"

#include <algorithm>
#include <sstream>

namespace ds2 {
namespace GDBRemote {

template <typename T> uint8_t Checksum(T const &data) {
  uint8_t csum = 0;
  for (char n : data) {
    csum += n;
  }
  return csum;
}

template <typename T> std::string Escape(T const &data) {
  std::ostringstream ss;
  auto first = data.begin();
  auto last = data.begin();
  static std::string const searchStr = "$#}*";

  while ((last = std::find_first_of(first, data.end(), searchStr.begin(),
                                    searchStr.end())) != data.end()) {
    while (first < last) {
      ss << *first++;
    }
    ss << '}' << static_cast<char>(*last - 0x20);
    first = ++last;
  }

  while (first < data.end()) {
    ss << *first++;
  }

  return ss.str();
}

template <typename T> std::string Unescape(T const &data) {
  std::ostringstream ss;
  auto first = data.begin();
  auto last = data.begin();

  while ((last = std::find(first, data.end(), '}')) != data.end()) {
    while (first < last) {
      ss << *first++;
    }
    ss << static_cast<char>(*(last + 1) + 0x20);
    last += 2, first = last;
  }

  while (first < data.end()) {
    ss << *first++;
  }

  return ss.str();
}
} // namespace GDBRemote
} // namespace ds2
