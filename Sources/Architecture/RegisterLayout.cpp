//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/RegisterLayout.h"
#include "DebugServer2/Utils/Log.h"

#include <cstring>
#include <cstdlib>
#include <sstream>

using ds2::Architecture::FlagDef;
using ds2::Architecture::FlagSet;
using ds2::Architecture::GDBDescriptor;
using ds2::Architecture::GDBEncoding;
using ds2::Architecture::GDBFeature;
using ds2::Architecture::GDBFeatureEntry;
using ds2::Architecture::GDBVectorDef;
using ds2::Architecture::GDBVectorUnion;
using ds2::Architecture::GDBVectorUnionField;
using ds2::Architecture::LLDBRegisterSet;
using ds2::Architecture::RegisterDef;

namespace {

class GDBXMLGenerator {
private:
  GDBDescriptor const *_desc;

public:
  GDBXMLGenerator(GDBDescriptor const &desc);

public:
  std::string main() const;
  std::string file(std::string const &filename) const;
  std::string feature(std::string const &identifier) const;
  std::string feature(size_t index) const;

private:
  void headers(std::ostringstream &s) const;

private:
  void generateEntry(std::ostringstream &s, GDBFeatureEntry const &entry) const;
  void generateRegister(std::ostringstream &s, RegisterDef const *def) const;
  void generateVector(std::ostringstream &s, GDBVectorDef const *def) const;
  void generateUnion(std::ostringstream &s, GDBVectorUnion const *def) const;
  void generateFlags(std::ostringstream &s, FlagSet const *set) const;

private:
  static std::string Escape(std::string const &s);
  static std::string Escape(int64_t value);

  static std::string Quote(std::string const &s);
  static std::string Quote(int64_t value);

  static std::string GetType(GDBEncoding const &enc, char const *name,
                             size_t bitsize);
  static std::string GetVectorType(GDBEncoding const &enc, size_t elsize);
  static size_t GetVectorCount(GDBEncoding const &enc, size_t vecsize,
                               size_t elsize);
};
}

GDBXMLGenerator::GDBXMLGenerator(GDBDescriptor const &desc) : _desc(&desc) {}

void GDBXMLGenerator::headers(std::ostringstream &s) const {
  s << "<?xml version=" << Quote("1.0") << "?>" << std::endl;
  s << "<!DOCTYPE target SYSTEM " << Quote("gdb-target.dtd") << ">"
    << std::endl;
}

std::string GDBXMLGenerator::Escape(std::string const &s) {
  std::ostringstream os;

  if (s.find_first_of("<>&\"") != std::string::npos) {
    for (size_t n = 0; n < s.length(); n++) {
      switch (s[n]) {
      case '<':
        os << "&lt;";
        break;
      case '>':
        os << "&gt;";
        break;
      case '&':
        os << "&amp;";
        break;
      case '"':
        os << "&quot;";
        break;
      default:
        os << s[n];
        break;
      }
    }
  } else {
    os << s;
  }

  return os.str();
}

std::string GDBXMLGenerator::Escape(int64_t value) {
  std::ostringstream os;
  os << value;
  return os.str();
}

std::string GDBXMLGenerator::Quote(std::string const &s) {
  std::ostringstream os;
  os << '"' << Escape(s) << '"';
  return os.str();
}

std::string GDBXMLGenerator::Quote(int64_t value) {
  std::ostringstream os;
  os << '"' << value << '"';
  return os.str();
}

std::string GDBXMLGenerator::main() const {
  std::ostringstream s;

  headers(s);

  s << "<target>" << std::endl;
  if (_desc->Architecture != nullptr &&
      strchr(_desc->Architecture, ':') != nullptr) {
    s << '\t' << "<architecture>" << Escape(_desc->Architecture)
      << "</architecture>" << std::endl;
  }
  if (_desc->OSABI != nullptr) {
    s << '\t' << "<osabi>" << Escape(_desc->OSABI) << "</osabi>" << std::endl;
  }
  for (size_t n = 0; n < _desc->Count; n++) {
    GDBFeature const *feature = _desc->Features[n];

    if (feature->FileName == nullptr) {
      s << '\t' << "<feature name=" << Quote(feature->Identifier) << "/>"
        << std::endl;
    } else {
      s << '\t' << "<xi:include href=" << Quote(feature->FileName) << "/>"
        << std::endl;
    }
  }
  s << "</target>" << std::endl;

  return s.str();
}

std::string GDBXMLGenerator::file(std::string const &filename) const {
  if (!filename.empty()) {
    for (size_t n = 0; n < _desc->Count; n++) {
      if (_desc->Features[n]->FileName != nullptr &&
          _desc->Features[n]->FileName == filename)
        return feature(n);
    }
  }

  return std::string();
}

std::string GDBXMLGenerator::feature(std::string const &identifier) const {
  if (!identifier.empty()) {
    for (size_t n = 0; n < _desc->Count; n++) {
      if (_desc->Features[n]->Identifier == identifier)
        return feature(n);
    }
  }

  return std::string();
}

std::string GDBXMLGenerator::GetType(GDBEncoding const &enc,
                                     char const *encname, size_t bitsize) {
  switch (enc) {
  case ds2::Architecture::kGDBEncodingIEEESingle:
    return "ieee_single";
  case ds2::Architecture::kGDBEncodingIEEEDouble:
    return "ieee_double";
  case ds2::Architecture::kGDBEncodingUInt128:
    return "uint128";
  case ds2::Architecture::kGDBEncodingX87Extension:
    return "i387_ext";
  case ds2::Architecture::kGDBEncodingDataPointer:
    return "data_ptr";
  case ds2::Architecture::kGDBEncodingCodePointer:
    return "code_ptr";
  case ds2::Architecture::kGDBEncodingCustom:
    if (encname != nullptr && *encname != '\0')
      return encname;
    break;
  case ds2::Architecture::kGDBEncodingInteger:
    return "int";
  case ds2::Architecture::kGDBEncodingSizedInteger:
    if (bitsize != 0)
      return "int" + Escape(bitsize);
    break;
  default:
    DS2ASSERT(0 && "invalid GDB encoding");
  }

  return std::string();
}

std::string GDBXMLGenerator::GetVectorType(GDBEncoding const &enc,
                                           size_t elsize) {
  switch (enc) {
  case ds2::Architecture::kGDBEncodingIEEESingle:
    return "ieee_single";
  case ds2::Architecture::kGDBEncodingIEEEDouble:
    return "ieee_double";
  case ds2::Architecture::kGDBEncodingInteger:
  case ds2::Architecture::kGDBEncodingSizedInteger:
    if (elsize != 0)
      return "int" + Escape(elsize);
    break;
  default:
    DS2ASSERT(0 && "invalid encoding for GDB vector");
  }

  return std::string();
}

size_t GDBXMLGenerator::GetVectorCount(GDBEncoding const &enc, size_t vecsize,
                                       size_t elsize) {
  switch (enc) {
  case ds2::Architecture::kGDBEncodingIEEESingle:
    elsize = 32;
    break;
  case ds2::Architecture::kGDBEncodingIEEEDouble:
    elsize = 64;
    break;
  case ds2::Architecture::kGDBEncodingInteger:
  case ds2::Architecture::kGDBEncodingSizedInteger:
    break;
  default:
    DS2ASSERT(0 && "invalid encoding for GDB vector");
  }

  return vecsize / elsize;
}

void GDBXMLGenerator::generateVector(std::ostringstream &s,
                                     GDBVectorDef const *def) const {
  s << '\t' << "<vector id=" << Quote(def->Name) << ' ';
  s << "type=" << Quote(GetVectorType(def->Encoding, def->ElementBitSize))
    << ' ';
  s << "count=" << Quote(GetVectorCount(def->Encoding, def->BitSize,
                                        def->ElementBitSize)) << ' ';
  s << "/>" << std::endl;
}

void GDBXMLGenerator::generateUnion(std::ostringstream &s,
                                    GDBVectorUnion const *def) const {
  s << '\t' << "<union id=" << Quote(def->Name) << '>' << std::endl;
  for (size_t n = 0; n < def->Count; n++) {
    GDBVectorUnionField const &fld = def->Fields[n];
    s << "\t\t"
      << "<field name=" << Quote(fld.Name) << ' ' << "type="
      << Quote(GetType(fld.Encoding,
                       fld.Def != nullptr ? fld.Def->Name : nullptr, 0)) << ' '
      << "/>" << std::endl;
  }
  s << '\t' << "</union>" << std::endl;
}

void GDBXMLGenerator::generateFlags(std::ostringstream &s,
                                    FlagSet const *set) const {
  s << '\t' << "<flags id=" << Quote(set->Name)
    << " size=" << Quote(set->BitSize >> 3) << '>' << std::endl;
  for (size_t n = 0; n < set->Count; n++) {
    FlagDef const &def = set->Defs[n];
    s << "\t\t"
      << "<field name=" << Quote(def.Name) << ' '
      << "start=" << Quote(def.Start) << ' '
      << "end=" << Quote(def.Start + def.Length - 1) << ' ' << "/>"
      << std::endl;
  }
  s << '\t' << "</flags>" << std::endl;
}

void GDBXMLGenerator::generateRegister(std::ostringstream &s,
                                       RegisterDef const *def) const {
  s << '\t';
  s << "<reg name=" << Quote(def->Name) << ' ';
  if (!(def->BitSize < 0)) {
    s << "bitsize=" << Quote(def->BitSize) << ' ';
  }
  if (def->GDBEncoding.Encoding != ds2::Architecture::kGDBEncodingUnknown) {
    s << "type=" << Quote(GetType(def->GDBEncoding.Encoding,
                                  def->GDBEncoding.Name, def->BitSize)) << ' ';
  }
  if ((def->Flags & ds2::Architecture::kRegisterDefNoGDBRegisterNumber) == 0 &&
      !(def->GDBRegisterNumber < 0)) {
    s << "regnum=" << Quote(def->GDBRegisterNumber) << ' ';
  }
  if (def->GDBGroupName != nullptr) {
    s << "group=" << Quote(def->GDBGroupName) << ' ';
  }
  s << "/>" << std::endl;
}

void GDBXMLGenerator::generateEntry(std::ostringstream &s,
                                    GDBFeatureEntry const &entry) const {
  if (entry.Type == ds2::Architecture::kGDBFeatureTypeNone)
    return;

  if (entry.Type == ds2::Architecture::kGDBFeatureTypeRegister) {
    generateRegister(s, reinterpret_cast<RegisterDef const *>(entry.Data));
  } else if (entry.Type == ds2::Architecture::kGDBFeatureTypeVector) {
    generateVector(s, reinterpret_cast<GDBVectorDef const *>(entry.Data));
  } else if (entry.Type == ds2::Architecture::kGDBFeatureTypeUnion) {
    generateUnion(s, reinterpret_cast<GDBVectorUnion const *>(entry.Data));
  } else if (entry.Type == ds2::Architecture::kGDBFeatureTypeFlags) {
    generateFlags(s, reinterpret_cast<FlagSet const *>(entry.Data));
  } else {
    DS2ASSERT(0 && "unknown GDB feature entry type");
  }
}

std::string GDBXMLGenerator::feature(size_t index) const {
  if (index >= _desc->Count)
    return std::string();

  GDBFeature const *feature = _desc->Features[index];

  std::ostringstream s;
  headers(s);

  s << "<feature name=" << Quote(feature->Identifier) << ">" << std::endl;
  for (size_t n = 0; n < feature->Count; n++) {
    generateEntry(s, feature->Entries[n]);
  }
  s << "</feature>" << std::endl;

  return s.str();
}

//
// Public API
//
namespace ds2 {
namespace Architecture {

std::string GDBGenerateXMLMain(GDBDescriptor const &desc) {
  return GDBXMLGenerator(desc).main();
}

std::string GDBGenerateXMLFeatureByIndex(GDBDescriptor const &desc,
                                         size_t index) {
  return GDBXMLGenerator(desc).feature(index);
}

std::string GDBGenerateXMLFeatureByFileName(GDBDescriptor const &desc,
                                            std::string const &filename) {
  return GDBXMLGenerator(desc).file(filename);
}

std::string GDBGenerateXMLFeatureByIdentifier(GDBDescriptor const &desc,
                                              std::string const &ident) {
  return GDBXMLGenerator(desc).feature(ident);
}

bool LLDBGetRegisterInfo(LLDBDescriptor const &desc, size_t index,
                         LLDBRegisterInfo &info) {
  for (size_t nset = 0; nset < desc.Count; nset++) {
    LLDBRegisterSet const *set = desc.Sets[nset];
    if (index >= set->Count) {
      index -= set->Count;
      continue;
    }

    info.SetName = set->Name;
    info.Def = set->Defs[index];
    return true;
  }

  return false;
}

bool LLDBGetRegisterInfo(LLDBDescriptor const &desc, std::string const &name,
                         LLDBRegisterInfo &info) {
  if (name.empty())
    return false;

  for (size_t nset = 0; nset < desc.Count; nset++) {
    LLDBRegisterSet const *set = desc.Sets[nset];
    RegisterDef const *const *defs = set->Defs;
    for (size_t index = 0; defs[index] != nullptr; index++) {
      RegisterDef const *rd = defs[index];
      if ((rd->LLDBName != nullptr && rd->LLDBName == name) ||
          rd->Name == name) {
        info.SetName = set->Name;
        info.Def = rd;
        return true;
      }
    }
  }

  return false;
}
}
}
