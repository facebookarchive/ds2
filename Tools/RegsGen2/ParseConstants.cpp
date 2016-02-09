//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "ParseConstants.h"
#include "KeyValue.h"

bool ParseEncodingName(std::string const &name, Encoding &enc) {
  static KeyValue<Encoding> const sEncodings[] = {
      {"int", kEncodingUInteger},
      {"uint", kEncodingUInteger},
      {"sint", kEncodingSInteger},
      {"ieee-single", kEncodingIEEESingle},
      {"ieee-double", kEncodingIEEEDouble},
      {"ieee-extended", kEncodingIEEEExtended},
      {nullptr, kEncodingInvalid}};

  if (name.empty())
    return false;

  return FindKeyValue(sEncodings, name, enc);
}

bool ParseGDBEncodingName(std::string const &name, GDBEncoding &enc,
                          std::string &cname) {
  static KeyValue<GDBEncoding> const sEncodings[] = {
      {"none", kGDBEncodingInvalid},
      {"int", kGDBEncodingInt},
      {"ieee-single", kGDBEncodingIEEESingle},
      {"ieee-double", kGDBEncodingIEEEDouble},
      {"data-pointer", kGDBEncodingDataPointer},
      {"code-pointer", kGDBEncodingCodePointer},
      {"x87-extension", kGDBEncodingX87Extension},
      {"uint128", kGDBEncodingUInt128},
      {"union", kGDBEncodingUnion},
      {nullptr, kGDBEncodingInvalid}};

  if (name.empty())
    return false;

  if (!FindKeyValue(sEncodings, name, enc)) {
    enc = kGDBEncodingCustom;
    cname = name;
  }

  return true;
}

bool ParseFormatName(std::string const &name, Format &fmt) {
  static KeyValue<Format> const sFormats[] = {{"bin", kFormatBinary},
                                              {"dec", kFormatDecimal},
                                              {"hex", kFormatHexadecimal},
                                              {"float", kFormatFloat},
                                              {"vector", kFormatVector},
                                              {nullptr, kFormatInvalid}};

  if (name.empty())
    return false;

  return FindKeyValue(sFormats, name, fmt);
}

bool ParseLLDBVectorFormatName(std::string const &name, LLDBVectorFormat &fmt) {
  static KeyValue<LLDBVectorFormat> const sFormats[] = {
      {"vector-uint8", kLLDBVectorFormatUInt8},
      {"vector-sint8", kLLDBVectorFormatSInt8},
      {"vector-uint16", kLLDBVectorFormatUInt16},
      {"vector-sint16", kLLDBVectorFormatSInt16},
      {"vector-uint32", kLLDBVectorFormatUInt32},
      {"vector-sint32", kLLDBVectorFormatSInt32},
      {"vector-uint8", kLLDBVectorFormatUInt128},
      {"vector-float32", kLLDBVectorFormatFloat32},
      {nullptr, kLLDBVectorFormatNone}};

  if (name.empty())
    return false;

  return FindKeyValue(sFormats, name, fmt);
}
