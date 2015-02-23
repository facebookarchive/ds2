//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Utils_Utils_String_h
#define __DebugServer2_Utils_Utils_String_h

#include <sstream>
#include <string>

namespace ds2 {

// Android doesn't have std::to_string, define our own implementation.
template<typename T>
static inline std::string ToString(T val) {
  std::ostringstream os;
  os << val;
  return os.str();
}
}

#endif // !__DebugServer2_Utils_Utils_String_h
