//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Architecture/RegisterLayout.h"
#include "DebugServer2/Utils/Log.h"
#include "DebugServer2/Utils/String.h"

#include <cstdlib>
#include <cstring>
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
using ds2::Architecture::LLDBDescriptor;
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

  static std::string GetType(GDBEncoding const &enc, char const *encname,
                             size_t bitsize);
  static std::string GetVectorType(GDBEncoding const &enc, size_t elsize);
  static size_t GetVectorCount(GDBEncoding const &enc, size_t vecsize,
                               size_t elsize);
};

class LLDBXMLGenerator {
private:
  LLDBDescriptor const *_desc;

public:
  LLDBXMLGenerator(LLDBDescriptor const &desc);

public:
  std::string main() const;
};
} // namespace

static std::string escape(std::string const &s) {
  std::ostringstream os;

  if (s.find_first_of("<>&\"") != std::string::npos) {
    for (char n : s) {
      switch (n) {
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
        os << n;
        break;
      }
    }
  } else {
    os << s;
  }

  return os.str();
}

static std::string quote(std::string const &s) {
  std::ostringstream os;
  os << '"' << escape(s) << '"';
  return os.str();
}

static std::string quote(int64_t value) {
  std::ostringstream os;
  os << '"' << value << '"';
  return os.str();
}

static void headers(std::ostringstream &s) {
  s << "<?xml version=" << quote("1.0") << "?>" << std::endl;
  s << "<!DOCTYPE target SYSTEM " << quote("gdb-target.dtd") << ">"
    << std::endl;
}

GDBXMLGenerator::GDBXMLGenerator(GDBDescriptor const &desc) : _desc(&desc) {}

std::string GDBXMLGenerator::main() const {
  std::ostringstream s;

  headers(s);

  s << "<target>" << std::endl;
  if (_desc->Architecture != nullptr &&
      strchr(_desc->Architecture, ':') != nullptr) {
    s << '\t' << "<architecture>" << escape(_desc->Architecture)
      << "</architecture>" << std::endl;
  }
  if (_desc->OSABI != nullptr) {
    s << '\t' << "<osabi>" << escape(_desc->OSABI) << "</osabi>" << std::endl;
  }

  for (size_t n = 0; n < _desc->Count; n++) {
    GDBFeature const *feature = _desc->Features[n];

    if (feature->FileName == nullptr) {
      s << '\t' << "<feature name=" << quote(feature->Identifier) << "/>"
        << std::endl;
    } else {
      s << '\t' << "<xi:include href=" << quote(feature->FileName) << "/>"
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
      return "int" + ds2::Utils::ToString(bitsize);
    break;
  default:
    DS2BUG("invalid GDB encoding");
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
      return "int" + ds2::Utils::ToString(elsize);
    break;
  default:
    DS2BUG("invalid encoding for GDB vector");
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
    DS2BUG("invalid encoding for GDB vector");
  }

  return vecsize / elsize;
}

void GDBXMLGenerator::generateVector(std::ostringstream &s,
                                     GDBVectorDef const *def) const {
  s << '\t' << "<vector id=" << quote(def->Name) << ' ';
  s << "type=" << quote(GetVectorType(def->Encoding, def->ElementBitSize))
    << ' ';
  s << "count="
    << quote(GetVectorCount(def->Encoding, def->BitSize, def->ElementBitSize))
    << ' ';
  s << "/>" << std::endl;
}

void GDBXMLGenerator::generateUnion(std::ostringstream &s,
                                    GDBVectorUnion const *def) const {
  s << '\t' << "<union id=" << quote(def->Name) << '>' << std::endl;
  for (size_t n = 0; n < def->Count; n++) {
    GDBVectorUnionField const &fld = def->Fields[n];
    s << "\t\t"
      << "<field name=" << quote(fld.Name) << ' ' << "type="
      << quote(GetType(fld.Encoding,
                       fld.Def != nullptr ? fld.Def->Name : nullptr, 0))
      << ' ' << "/>" << std::endl;
  }
  s << '\t' << "</union>" << std::endl;
}

void GDBXMLGenerator::generateFlags(std::ostringstream &s,
                                    FlagSet const *set) const {
  s << '\t' << "<flags id=" << quote(set->Name)
    << " size=" << quote(set->BitSize >> 3) << '>' << std::endl;
  for (size_t n = 0; n < set->Count; n++) {
    FlagDef const &def = set->Defs[n];
    s << "\t\t"
      << "<field name=" << quote(def.Name) << ' '
      << "start=" << quote(def.Start) << ' '
      << "end=" << quote(def.Start + def.Length - 1) << ' ' << "/>"
      << std::endl;
  }
  s << '\t' << "</flags>" << std::endl;
}

void GDBXMLGenerator::generateRegister(std::ostringstream &s,
                                       RegisterDef const *def) const {
  s << '\t';
  s << "<reg name=" << quote(def->Name) << ' ';
  if (!(def->BitSize < 0)) {
    s << "bitsize=" << quote(def->BitSize) << ' ';
  }
  if (def->GDBEncoding.Encoding != ds2::Architecture::kGDBEncodingUnknown) {
    s << "type="
      << quote(GetType(def->GDBEncoding.Encoding, def->GDBEncoding.Name,
                       def->BitSize))
      << ' ';
  }
  if ((def->Flags & ds2::Architecture::kRegisterDefNoGDBRegisterNumber) == 0 &&
      !(def->GDBRegisterNumber < 0)) {
    s << "regnum=" << quote(def->GDBRegisterNumber) << ' ';
  }
  if (def->GDBGroupName != nullptr) {
    s << "group=" << quote(def->GDBGroupName) << ' ';
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
    DS2BUG("unknown GDB feature entry type");
  }
}

std::string GDBXMLGenerator::feature(size_t index) const {
  if (index >= _desc->Count)
    return std::string();

  GDBFeature const *feature = _desc->Features[index];

  std::ostringstream s;
  headers(s);

  s << "<feature name=" << quote(feature->Identifier) << ">" << std::endl;
  for (size_t n = 0; n < feature->Count; n++) {
    generateEntry(s, feature->Entries[n]);
  }
  s << "</feature>" << std::endl;

  return s.str();
}

LLDBXMLGenerator::LLDBXMLGenerator(LLDBDescriptor const &desc) : _desc(&desc) {}

std::string LLDBXMLGenerator::main() const {
  std::ostringstream s;

  headers(s);

  s << "<target>" << std::endl;

  for (size_t nset = 0; nset < _desc->Count; nset++) {
    LLDBRegisterSet const *set = _desc->Sets[nset];
    s << "\t<include href=" << quote(set->Name) << "/>" << std::endl;
  }

  s << '\t' << "<groups>" << '\n';
  for (size_t nset = 0; nset < _desc->Count; nset++) {
    LLDBRegisterSet const *set = _desc->Sets[nset];
    s << "\t\t<group id=" << quote(nset) << " name=" << quote(set->Name) << "/>"
      << std::endl;
  }
  s << "\t</groups>" << std::endl;

  s << "</target>" << std::endl;

  return s.str();
}

//
// Public API
//
namespace ds2 {
namespace Architecture {

std::string GenerateXMLHeader() {
  std::ostringstream s;
  headers(s);
  return s.str();
}

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

std::string LLDBGenerateXMLMain(LLDBDescriptor const &desc) {
  return LLDBXMLGenerator(desc).main();
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
} // namespace Architecture
} // namespace ds2
