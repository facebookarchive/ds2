//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#pragma once

#include "DebugServer2/Base.h"
#include "DebugServer2/Types.h"

#include <map>

namespace ds2 {
namespace Architecture {

enum Encoding {
  kEncodingNone,
  kEncodingUInteger,
  kEncodingSInteger,
  kEncodingIEEESingle,
  kEncodingIEEEDouble,
  kEncodingIEEEExtended
};

enum Format {
  kFormatNone,
  kFormatBinary,
  kFormatDecimal,
  kFormatHexadecimal,
  kFormatFloat,
  kFormatVector
};

enum GDBEncoding {
  kGDBEncodingNone,
  kGDBEncodingInteger,
  kGDBEncodingSizedInteger,
  kGDBEncodingUInt128,
  kGDBEncodingDataPointer,
  kGDBEncodingCodePointer,
  kGDBEncodingX87Extension,
  kGDBEncodingIEEESingle,
  kGDBEncodingIEEEDouble,
  kGDBEncodingCustom,
  kGDBEncodingUnknown
};

enum GDBFeatureEntryType {
  kGDBFeatureTypeNone,
  kGDBFeatureTypeRegister,
  kGDBFeatureTypeVector,
  kGDBFeatureTypeFlags,
  kGDBFeatureTypeUnion
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

enum /*RegisterDefFlags*/ { kRegisterDefNoGDBRegisterNumber = (1 << 0) };

struct RegisterDef {
  char const *Name;
  char const *LLDBName;
  char const *AlternateName;
  char const *GenericName;
  char const *GDBGroupName;
  ssize_t BitSize;
  int32_t DWARFRegisterNumber;
  int32_t GDBRegisterNumber;
  int32_t EHFrameRegisterNumber;
  int32_t LLDBRegisterNumber;
  ssize_t LLDBOffset;
  Architecture::LLDBVectorFormat LLDBVectorFormat;
  Architecture::Encoding Encoding;
  Architecture::Format Format;
  struct {
    Architecture::GDBEncoding Encoding;
    char const *Name;
  } GDBEncoding;
  uint32_t Flags;
  RegisterDef const *const *InvalidatedRegisters;
  RegisterDef const *const *ContainerRegisters;
};

struct FlagDef {
  char const *Name;
  size_t Start;
  size_t Length;
};

struct FlagSet {
  char const *Name;
  size_t BitSize;
  size_t Count;
  FlagDef const *Defs;
};

struct GDBVectorDef {
  char const *Name;
  size_t BitSize;
  ssize_t ElementBitSize;
  GDBEncoding Encoding;
};

struct GDBVectorUnionField {
  char const *Name;
  GDBEncoding Encoding;
  GDBVectorDef const *Def;
};

struct GDBVectorUnion {
  char const *Name;
  size_t Count;
  GDBVectorUnionField const *Fields;
};

struct GDBFeatureEntry {
  GDBFeatureEntryType Type;
  void const *Data;
};

struct GDBFeature {
  char const *Identifier;
  char const *FileName;
  size_t Count;
  GDBFeatureEntry const *Entries;
};

struct LLDBRegisterSet {
  char const *Name;
  size_t Count;
  RegisterDef const *const *Defs;
};

struct GDBDescriptor {
  char const *Architecture;
  char const *OSABI;
  size_t Count;
  GDBFeature const *const *Features;
};

struct LLDBDescriptor {
  size_t Count;
  LLDBRegisterSet const *const *Sets;
};

//
// Public APIs
//

struct GPRegisterValue {
  size_t size;
  uint64_t value;
};

typedef std::map<size_t, GPRegisterValue> GPRegisterStopMap;
typedef std::vector<GPRegisterValue> GPRegisterValueVector;

//
// XML Generation Functions
//

std::string GenerateXMLHeader();

std::string GDBGenerateXMLMain(GDBDescriptor const &desc);

std::string GDBGenerateXMLFeatureByIndex(GDBDescriptor const &desc,
                                         size_t index);

std::string GDBGenerateXMLFeatureByFileName(GDBDescriptor const &desc,
                                            std::string const &filename);

std::string GDBGenerateXMLFeatureByIdentifier(GDBDescriptor const &desc,
                                              std::string const &ident);

std::string LLDBGenerateXMLMain(LLDBDescriptor const &lldbDesc);

//
// LLDB Register Information Functions
//

struct LLDBRegisterInfo {
  char const *SetName;
  RegisterDef const *Def;
};

bool LLDBGetRegisterInfo(LLDBDescriptor const &desc, size_t index,
                         LLDBRegisterInfo &info);

bool LLDBGetRegisterInfo(LLDBDescriptor const &desc, std::string const &name,
                         LLDBRegisterInfo &info);
} // namespace Architecture
} // namespace ds2
