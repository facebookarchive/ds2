//
// Copyright (c) 2015, Corentin Derbois <cderbois@gmail.com>.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Support_CompressionBase_h
#define __DebugServer2_Support_CompressionBase_h

#include "DebugServer2/Base.h"
#include "DebugServer2/Constants.h"

#include <string>

namespace ds2 {
namespace Support {

class CompressionBase {

public:
  enum Algo { kNone, kLz4, kLzfse, kLzma, kZlibDeflate };

public:
  CompressionBase();
  virtual ~CompressionBase();

public:
  virtual bool isEnable(void) const { return _enable; }
  virtual void enable(bool enable) { _enable = enable; }

public:
  virtual ErrorCode setAlgo(std::string algo, int min);

public:
  virtual std::string compress(const std::string &source);

protected:
  static const int defaultMin = 384; /* lldb default min */

protected:
  bool _enable;
  Algo _algo;
  int _min;
};
}
}

#endif // !__DebugServer2_Support_CompressionBase_h
