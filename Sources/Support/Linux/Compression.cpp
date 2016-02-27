//
// Copyright (c) 2015, Corentin Derbois <cderbois@gmail.com>.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Support/Linux/Compression.h"
#include "DebugServer2/Utils/Log.h"

#include <unistd.h>
#include <vector>
#include <sstream>
#include <cstring>
#include <zlib.h>

namespace ds2 {
namespace Support {
namespace Linux {

std::string Compression::getSupported(void) {
  std::ostringstream output;

  output << "qXfer:features:read+;";
  output << "SupportedCompressions=zlib-deflate;";
  output << "DefaultCompressionMinSize=" << defaultMin;

  return output.str();
}

ErrorCode Compression::setAlgo(std::string algo, int min) {
  ErrorCode err;

  err = super::setAlgo(algo, min);
  if (err != kSuccess)
    return err;

  /*
   * we just support one algo for the moment so
   * if lldb return an other algo that we propose
   * let's catch that here.
   */

  if (_algo != kZlibDeflate)
    return kErrorUnsupported;

  return kSuccess;
}

std::string Compression::compress(const std::string &source) {
  /*
   * The size of the metadate for Zlib is 10 bytes. Let's add
   * this size to the source size. This will insure that the
   * output buffer is big enough.
   */
  std::vector<uint8_t> compressed_buf(source.size() + zLibMetaDataSize);
  std::ostringstream ss;
  std::string compressed;
  z_stream stream;
  int state;

  if (source.size() < (uint32_t)_min) {
    // Uncompressed format $N<uncompressed payload>#00
    return "N" + source;
  }

  ::memset(&stream, 0, sizeof(z_stream));

  stream.next_in = (Bytef *)source.c_str();
  stream.avail_in = (uInt)source.size();
  stream.next_out = (Bytef *)compressed_buf.data();
  stream.avail_out = (uInt)source.size() + zLibMetaDataSize;
  stream.zalloc = Z_NULL;
  stream.zfree = Z_NULL;
  stream.opaque = Z_NULL;
  deflateInit2(&stream, 5, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);

  state = deflate(&stream, Z_FINISH);
  if (state != Z_STREAM_END && stream.avail_out == 0) {
    // Uncompressed format $N<uncompressed payload>#00
    return "N" + source;
  }

  // format: $C<size of uncompressed payload in base10>:<compressed payload>#00
  ss << "C" << source.size() << ":";
  compressed = ss.str();

  for (size_t idx = 0; idx < stream.total_out; idx++) {
    uint8_t byte = compressed_buf[idx];

    if (byte == '\0' || byte == '}' || byte == '#' || byte == '$' ||
        byte == '*') {
      compressed.push_back(0x7D);
      compressed.push_back(byte ^ 0x20);
    } else {
      compressed.push_back(byte);
    }
  }

  return compressed;
}
}
}
}
