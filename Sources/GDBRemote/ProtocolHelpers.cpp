//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/GDBRemote/ProtocolHelpers.h"

#include <sstream>

namespace ds2 {
namespace GDBRemote {

uint8_t Checksum(std::string const &data) {
  uint8_t csum = 0;
  for (char n : data) {
    csum += n;
  }
  return csum;
}

std::string Escape(std::string const &data) {
  std::ostringstream ss;
  size_t first = 0;
  size_t last = 0;
  size_t count = 0;
  size_t lastp = std::string::npos;

  while ((last = data.find_first_of("$#}*", last)) != std::string::npos) {
    ss << data.substr(first, last - first);
    ss << '}' << static_cast<char>(data[last] - 0x20);
    first = ++last, lastp = last, count++;
  }
  if (lastp != std::string::npos && lastp < data.length()) {
    ss << data.substr(lastp);
    count++;
  }

  if (count == 0)
    return data;

  return ss.str();
}

std::string Unescape(std::string const &data) {
  std::ostringstream ss;
  size_t first = 0;
  size_t last = 0;
  size_t count = 0;
  size_t lastp = std::string::npos;

  while ((last = data.find('}', last)) != std::string::npos) {
    ss << data.substr(first, last - first);
    ss << static_cast<char>(data[last + 1] + 0x20);
    last += 2, first = last, lastp = last, count++;
  }
  if (lastp != std::string::npos && lastp < data.length()) {
    ss << data.substr(lastp);
    count++;
  }

  if (count == 0)
    return data;

  return ss.str();
}

std::string Uncompress(std::string const &data) { return Unescape(data); }
}
}
