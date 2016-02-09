//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Support_Windows_Stringify_h
#define __DebugServer2_Support_Windows_Stringify_h

#ifndef __DebugServer2_Support_Stringify_h
#error "You shall not include this file directly."
#endif

#include <windows.h>

namespace ds2 {
namespace Support {
namespace Windows {

class Stringify {
public:
  static char const *ExceptionCode(DWORD code, bool dieOnFail = true);
};
}
}
}

#endif // !__DebugServer2_Support_Windows_Stringify_h
