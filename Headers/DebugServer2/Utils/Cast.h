//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Utils_Cast_h
#define __DebugServer2_Utils_Cast_h

#include <cstddef>

namespace ds2 {
namespace Utils {

// Utility functions to allow nullptr to be used with wrapPtrace
static inline uintptr_t castToInt(std::nullptr_t ptr) { return 0; }

static inline uintptr_t castToInt(uintptr_t ptr) { return ptr; }
}
}

#endif // !__DebugServer2_Utils_Bits_h
