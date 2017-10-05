//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __regsgen_Definitions_h
#define __regsgen_Definitions_h

#include "Constants.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <sys/types.h>
#include <vector>

struct Register {
  typedef std::shared_ptr<Register> shared_ptr;
  typedef std::vector<shared_ptr> vector;
  typedef std::set<shared_ptr> set;
  typedef std::map<std::string, shared_ptr> name_map;

  size_t Index;

  bool Private;
  bool NoGDBRegisterNumber;

  ssize_t BitSize;
  ::Format Format;
  ::LLDBVectorFormat LLDBVectorFormat;
  ::Encoding Encoding;
  ::GDBEncoding GDBEncoding;
  std::string GDBEncodingName;

  ssize_t GDBRegisterNumber;
  ssize_t EHFrameRegisterNumber;
  ssize_t DWARFRegisterNumber;
  ssize_t LLDBRegisterNumber;

  std::string Name;
  std::string CName;
  std::string LLDBName;
  std::string AlternateName;
  std::string GenericName;

  std::string GDBGroup;

  ssize_t LLDBOffset;

  vector InvalidateRegisters;
  vector ContainerRegisters;

  std::vector<std::string> InvalidateRegisterNames;
  std::vector<std::string> ContainerRegisterNames;
  std::set<std::string> ReferencingSets;

  std::string ParentSetName;
  std::string ParentRegisterName;
  ssize_t ParentElement;
  shared_ptr ParentRegister;

  Register()
      : Index(0), Private(false), NoGDBRegisterNumber(false), BitSize(0),
        Format(kFormatInvalid), LLDBVectorFormat(kLLDBVectorFormatNone),
        Encoding(kEncodingInvalid), GDBEncoding(kGDBEncodingInvalid),
        GDBRegisterNumber(-1), EHFrameRegisterNumber(-1), DWARFRegisterNumber(-1),
        LLDBRegisterNumber(-1), LLDBOffset(-1), ParentElement(-1) {}
};

struct Flag {
  typedef std::shared_ptr<Flag> shared_ptr;
  typedef std::vector<shared_ptr> vector;

  Flag(const std::string name, size_t start, size_t length)
      : Name(name), Start(start), Length(length) {}
  Flag() = default;

  std::string Name;
  size_t Start;
  size_t Length;
};

struct GDBUnion {
  struct Field {
    typedef std::vector<Field> vector;

    std::string Name;
    ::GDBEncoding Encoding;
    std::string EncodingName;
  };

  Field::vector FieldNames;
};

struct GDBVector {
  typedef std::shared_ptr<GDBVector> shared_ptr;
  typedef std::vector<shared_ptr> vector;
  typedef std::map<std::string, shared_ptr> name_map;

  size_t BitSize;
  ssize_t ElementSize;

  std::string Name;
  ::GDBEncoding Encoding;
  ::GDBUnion Union;

  GDBVector() : BitSize(0), ElementSize(-1), Encoding(kGDBEncodingInvalid) {}
};

#endif // !__regsgen_Definitions_h
