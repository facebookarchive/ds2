// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

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
