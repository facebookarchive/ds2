//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __regsgen_Constants_h
#define __regsgen_Constants_h

enum Encoding {
  kEncodingInvalid,
  kEncodingUInteger,
  kEncodingSInteger,
  kEncodingIEEESingle,
  kEncodingIEEEDouble,
  kEncodingIEEEExtended
};

enum GDBEncoding {
  kGDBEncodingInvalid,
  kGDBEncodingInt,
  kGDBEncodingIEEESingle,
  kGDBEncodingIEEEDouble,
  kGDBEncodingDataPointer,
  kGDBEncodingCodePointer,
  kGDBEncodingX87Extension,
  kGDBEncodingUInt128,
  kGDBEncodingUnion,
  kGDBEncodingCustom
};

enum Format {
  kFormatInvalid,
  kFormatBinary,
  kFormatDecimal,
  kFormatHexadecimal,
  kFormatFloat,
  kFormatVector
};

enum LLDBVectorFormat {
  kLLDBVectorFormatNone,
  kLLDBVectorFormatUInt8,
  kLLDBVectorFormatSInt8,
  kLLDBVectorFormatUInt16,
  kLLDBVectorFormatSInt16,
  kLLDBVectorFormatUInt32,
  kLLDBVectorFormatSInt32,
  kLLDBVectorFormatUInt64,
  kLLDBVectorFormatSInt64,
  kLLDBVectorFormatUInt128,
  kLLDBVectorFormatFloat32
};

#endif // !__regsgen_Constants_h
