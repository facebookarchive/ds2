//
// Copyright (c) 2015, Corentin Derbois <cderbois@gmail.com>.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Support_Linux_Compression_h
#define __DebugServer2_Support_Linux_Compression_h

#include "DebugServer2/Base.h"
#include "DebugServer2/Constants.h"
#include "DebugServer2/Support/CompressionBase.h"

namespace ds2 {
namespace Support {
namespace Linux {

#define super Support::CompressionBase
#define zLibMetaDataSize 10

class Compression : public Support::CompressionBase {

public:
  static std::string getSupported(void);

public:
  virtual ErrorCode setAlgo(std::string algo, int min);

public:
  virtual std::string compress(const std::string &source);
};
}
}
}

#endif // !__DebugServer2_Support_Linux_Compression_h
