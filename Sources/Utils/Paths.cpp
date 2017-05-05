//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Utils/Paths.h"

namespace ds2 {
namespace Utils {

std::string Basename(std::string const &path) {
#if defined(OS_WIN32)
  char sep = '\\';
#else
  char sep = '/';
#endif

  auto end = path.find_last_not_of(sep);
  if (end == std::string::npos)
    return std::string(&sep, 1);

  auto begin = path.rfind(sep, end);
  if (begin == std::string::npos) {
    return path.substr(0, end + 1);
  } else {
    return path.substr(begin + 1, end - begin);
  }
}
} // namespace Utils
} // namespace ds2
