//
// Copyright (c) 2015, Corentin Derbois <cderbois@gmail.com>.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Support/CompressionBase.h"
#include "DebugServer2/Utils/Log.h"

#include <string>

namespace ds2 {
namespace Support {

CompressionBase::CompressionBase() : _enable(false), _algo(kNone), _min(0) {}

CompressionBase::~CompressionBase() {}

ErrorCode CompressionBase::setAlgo(std::string algo, int min) {
  _algo = kNone;

  if (algo == "lzfse")
    _algo = kLzfse;
  else if (algo == "zlib-deflate")
    _algo = kZlibDeflate;
  else if (algo == "lz4")
    _algo = kLz4;
  else if (algo == "lzma")
    _algo = kLzma;

  if (_algo == kNone)
    return kErrorInvalidArgument;

  if (min != -1)
    _min = min;

  return kSuccess;
}

std::string CompressionBase::compress(const std::string &source) {
  std::string compressed;

  DS2BUG("not implemented");

  return compressed.assign(source);
}
}
}
