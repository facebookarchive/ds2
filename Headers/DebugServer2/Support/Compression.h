//
// Copyright (c) 2015, Corentin Derbois <cderbois@gmail.com>.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Support_Compression_h
#define __DebugServer2_Support_Compression_h

#include "DebugServer2/Base.h"
#include "DebugServer2/Constants.h"
#include "DebugServer2/Support/CompressionBase.h"

#if defined(OS_LINUX)
#include "DebugServer2/Support/Linux/Compression.h"
#endif

namespace ds2 {
namespace Support {

#if defined(OS_LINUX)
using Linux::Compression;
#else
class Compression : public CompressionBase {

public:
  static std::string getSupported(void) { return nullptr; }
};
#endif
}
}

#endif // !__DebugServer2_Support_Compression_h
