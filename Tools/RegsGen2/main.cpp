//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "Context.h"

#include <cerrno>
#include <cstring>
#include <sstream>
#include <unistd.h>

static char const *const sKnownOSABI[] = {"linux", "freebsd", "darwin",
                                          "windows", nullptr};

static char const *GetGenericOSABIName(char const *osabi) {
  if (osabi == nullptr || strcmp(osabi, "windows") != 0)
    return "sysv";
  else
    return "windows";
}

static char const *GetLongOSABIName(char const *osabi) {
  if (osabi == nullptr)
    return nullptr;

  if (strcmp(osabi, "linux") == 0)
    return "GNU/Linux";
  else if (strcmp(osabi, "freebsd") == 0)
    return "FreeBSD";
  else if (strcmp(osabi, "netbsd") == 0)
    return "NetBSD";
  else if (strcmp(osabi, "openbsd") == 0)
    return "OpenBSD";
  else if (strcmp(osabi, "windows") == 0)
    return "Windows";
  else if (strcmp(osabi, "darwin") == 0)
    return "Darwin";
  else
    return nullptr;
}

//
// Process all flag sets in the definition file.
//
static bool ProcessFlagSets(Context &ctx, JSDictionary const *d) {
  for (auto name : *d) {
    if (ctx.FlagSets.find(name) != ctx.FlagSets.end()) {
      fprintf(stderr, "error: flag set '%s' is already defined\n",
              name.c_str());
      return false;
    }

    auto flagset = new FlagSet;
    if (!flagset->parse(name, d->value<JSDictionary>(name))) {
      delete flagset;
      return false;
    }

    ctx.FlagSets[name] = FlagSet::shared_ptr(flagset);
  }

  return true;
}

//
// Process all register sets in the definition file.
//
static bool ProcessRegisterSets(Context &ctx, JSDictionary const *d) {
  for (auto name : *d) {
    if (ctx.RegisterSets.find(name) != ctx.RegisterSets.end()) {
      fprintf(stderr, "error: register set '%s' is already defined\n",
              name.c_str());
      return false;
    }

    RegisterSet::shared_ptr regset(new RegisterSet);
    if (!regset->parse(ctx, name, d->value<JSDictionary>(name)))
      return false;

    ctx.RegisterSets[name] = regset;

    //
    // Now merge the registers into the global registers map,
    // we need to check that the register name is unique across
    // all the register sets, this is due to avoid conflicts in
    // the resulting C++ and Header files.
    //
    for (auto reg : *regset) {
      if (reg->Private)
        continue;

      if (ctx.PublicRegisters.find(reg->Name) != ctx.PublicRegisters.end()) {
        fprintf(stderr, "error: register '%s' in register set '%s' "
                        "is already defined\n",
                reg->Name.c_str(), regset->name().c_str());
        return false;
      }

      if (!reg->LLDBName.empty() &&
          ctx.PublicRegisters.find(reg->LLDBName) !=
              ctx.PublicRegisters.end()) {
        fprintf(stderr, "error: register '%s' in register set '%s' "
                        "is already defined\n",
                reg->LLDBName.c_str(), regset->name().c_str());
        return false;
      }

      reg->Index = ctx.PublicRegisters.size();
      ctx.PublicRegisters[reg->Name] = reg;
      if (!reg->LLDBName.empty()) {
        ctx.PublicRegisters[reg->LLDBName] = reg;
      }
    }
  }

  return true;
}

//
// Process the GDB vector set in the definition file.
//
static bool ProcessGDBVectorSet(Context &ctx, JSDictionary const *d) {
  return ctx.GDBVectorSet.parse(d);
}

static bool CheckRegisterSets(Context &ctx) {
  for (auto rs : ctx.RegisterSets) {
    if (!rs.second->finalize(ctx))
      return false;
  }

  return true;
}

//
// Assign names valid for C++ code.
//
static void AssignCNames(Context &ctx) {
  std::set<std::string> names;

  for (auto regdef : ctx.RegisterSets) {
    for (auto reg : *regdef.second) {
      if (names.find(reg->Name) != names.end()) {
        size_t index = 1;
        do {
          std::stringstream ss;
          ss << reg->Name << '_' << index;
          reg->CName = ss.str();
          index++;
        } while (names.find(reg->CName) != names.end());
      } else {
        reg->CName = reg->Name;
      }

      names.insert(reg->CName);
    }
  }
}

static void CollectAllRegisters(Context &ctx) {
  ctx.HasInvalidatedOrContainerSets = false;
  for (auto regset : ctx.RegisterSets) {
    for (auto reg : *regset.second) {
      if (!reg->InvalidateRegisters.empty() ||
          !reg->ContainerRegisters.empty()) {
        ctx.HasInvalidatedOrContainerSets = true;
      }
      ctx.Registers.insert(reg);
    }
  }
}

static void AssignLLDBIndexes(Context const &ctx) {
  if (ctx.LLDBDefs.empty())
    return;

  size_t count = 0;
  for (auto def : ctx.LLDBDefs) {
    for (auto regset : def->RegisterSets) {
      for (auto &reg : *regset) {
        reg->LLDBRegisterNumber = count++;
      }
    }
  }
}

//
// In order to compute the offsets the ParentSetName must
// be set, the entry must have a single entry in the
// ContainerRegisters array or a ParentRegisterName (which
// shall be part of ParentSetName). The ParentElement must
// be also set. Most of this is done in RegisterSet::finalize,
// since we cannot know the endian a priori, we'll emit a
// macro for those registers having a parent.
//
static void ComputeLLDBOffsets(Context const &ctx) {
  if (ctx.LLDBDefs.empty())
    return;

  //
  // Compute offsets only on registers lacking a parent.
  //
  size_t LLDBOffset = 0;
  for (auto def : ctx.LLDBDefs) {
    for (auto regset : def->RegisterSets) {
      for (auto &reg : *regset) {
        if (reg->ParentRegister == nullptr) {
          size_t nbytes = reg->BitSize >> 3;
          size_t align = nbytes;

          if (align > 16 || (align & (align - 1)) != 0) {
            if (align < 16) {
              align = 8;
            } else {
              align = 16;
            }
          }

          LLDBOffset = (LLDBOffset + align - 1) & -align;

          reg->LLDBOffset = LLDBOffset;
          LLDBOffset += nbytes;
        }
      }
    }
  }
}

//
// The main parsing function
//
static void Parse(std::string const &path, Context &ctx) {
  ctx.Path = path;
  ctx.HasInvalidatedOrContainerSets = false;

  auto root = JSDictionary::Parse(path, [&](unsigned line, unsigned column,
                                            std::string const &msg) -> bool {
    fprintf(stderr, "%s:%d:%d:error: %s\n", path.c_str(), line, column,
            msg.c_str());
    exit(EXIT_FAILURE);
  });

  if (root == nullptr) {
    fprintf(stderr, "%s: %s\n", path.c_str(), strerror(errno));
    exit(EXIT_FAILURE);
  }

  if (auto ns = root->value<JSString>("namespace")) {
    ctx.Namespace = ns->value();
  } else {
    fprintf(stderr, "error: mandatory namespace key is missing in global "
                    "dictionary\n");
    exit(EXIT_FAILURE);
  }

  if (auto d = root->value<JSDictionary>("flag-sets")) {
    if (!ProcessFlagSets(ctx, d)) {
      exit(EXIT_FAILURE);
    }
  }

  if (auto d = root->value<JSDictionary>("gdb-vector-set")) {
    if (!ProcessGDBVectorSet(ctx, d)) {
      exit(EXIT_FAILURE);
    }
  }

  if (auto d = root->value<JSDictionary>("register-sets")) {
    if (!ProcessRegisterSets(ctx, d) || !CheckRegisterSets(ctx)) {
      exit(EXIT_FAILURE);
    }
  }

  if (auto a = root->value<JSArray>("lldb-defs")) {
    if (!ctx.LLDBDefs.parse(ctx, a)) {
      exit(EXIT_FAILURE);
    }
  }

  if (auto d = root->value<JSDictionary>("gdb-defs")) {
    if (!ctx.GDBDefs.parse(ctx, d)) {
      exit(EXIT_FAILURE);
    }
  }

  AssignCNames(ctx);
  AssignLLDBIndexes(ctx);
  ComputeLLDBOffsets(ctx);
  CollectAllRegisters(ctx);
}

//
// Code Generation
//

static std::string QuoteOrNull(std::string const &s) {
  std::stringstream ss;
  if (s.empty())
    return "nullptr";

  ss << '"' << s << '"';
  return ss.str();
}

static char const *NameForEncoding(Encoding const &enc) {
  switch (enc) {
  case kEncodingUInteger:
    return "ds2::Architecture::kEncodingUInteger";
  case kEncodingSInteger:
    return "ds2::Architecture::kEncodingSInteger";
  case kEncodingIEEESingle:
    return "ds2::Architecture::kEncodingIEEESingle";
  case kEncodingIEEEDouble:
    return "ds2::Architecture::kEncodingIEEEDouble";
  case kEncodingIEEEExtended:
    return "ds2::Architecture::kEncodingIEEEExtended";
  default:
    break;
  }
  return "ds2::Architecture::kEncodingNone";
}

static char const *GDBNameForEncoding(Encoding const &enc) {
  switch (enc) {
  case kEncodingUInteger:
    return "ds2::Architecture::kGDBEncodingSizedInteger";
  case kEncodingSInteger:
    return "ds2::Architecture::kGDBEncodingSizedInteger";
  case kEncodingIEEESingle:
    return "ds2::Architecture::kGDBEncodingIEEESingle";
  case kEncodingIEEEDouble:
    return "ds2::Architecture::kGDBEncodingIEEEDouble";
  default:
    break;
  }
  return "ds2::Architecture::kGDBEncodingUnknown";
}

static char const *NameForGDBEncoding(GDBEncoding const &enc) {
  switch (enc) {
  case kGDBEncodingInt:
    return "ds2::Architecture::kGDBEncodingInteger";
  case kGDBEncodingIEEESingle:
    return "ds2::Architecture::kGDBEncodingIEEESingle";
  case kGDBEncodingIEEEDouble:
    return "ds2::Architecture::kGDBEncodingIEEEDouble";
  case kGDBEncodingDataPointer:
    return "ds2::Architecture::kGDBEncodingDataPointer";
  case kGDBEncodingCodePointer:
    return "ds2::Architecture::kGDBEncodingCodePointer";
  case kGDBEncodingX87Extension:
    return "ds2::Architecture::kGDBEncodingX87Extension";
  case kGDBEncodingUInt128:
    return "ds2::Architecture::kGDBEncodingUInt128";
  case kGDBEncodingCustom:
    return "ds2::Architecture::kGDBEncodingCustom";
  case kGDBEncodingUnion:
    return "ds2::Architecture::kGDBEncodingCustom";
  default:
    break;
  }
  return "ds2::Architecture::kGDBEncodingNone";
}

static char const *NameForFormat(Format const &fmt) {
  switch (fmt) {
  case kFormatBinary:
    return "ds2::Architecture::kFormatBinary";
  case kFormatDecimal:
    return "ds2::Architecture::kFormatDecimal";
  case kFormatHexadecimal:
    return "ds2::Architecture::kFormatHexadecimal";
  case kFormatFloat:
    return "ds2::Architecture::kFormatFloat";
  case kFormatVector:
    return "ds2::Architecture::kFormatVector";
  default:
    break;
  }
  return "ds2::Architecture::kFormatNone";
}

static std::string BuildGDBEncodingArray(Register const *reg) {
  std::stringstream ss;

  ss << '{' << ' ';
  if (reg->GDBEncoding == kGDBEncodingInvalid) {
    ss << GDBNameForEncoding(reg->Encoding);
  } else {
    ss << NameForGDBEncoding(reg->GDBEncoding);
  }
  ss << ',' << ' ' << QuoteOrNull(reg->GDBEncodingName) << ' ' << '}';

  return ss.str();
}

static std::string NameOfRegister(ssize_t regno, std::string const &name,
                                  std::string const &prefix) {
  if (regno < 0)
    return "-1";

  return prefix + name;
}

static std::string NameForLLDBVectorFormat(Format const &fmt,
                                           LLDBVectorFormat const &vfmt) {
  if (fmt != kFormatVector)
    return "ds2::Architecture::kLLDBVectorFormatNone";

  switch (vfmt) {
  case kLLDBVectorFormatUInt8:
    return "ds2::Architecture::kLLDBVectorFormatUInt8";
  case kLLDBVectorFormatSInt8:
    return "ds2::Architecture::kLLDBVectorFormatSInt8";
  case kLLDBVectorFormatUInt16:
    return "ds2::Architecture::kLLDBVectorFormatUInt16";
  case kLLDBVectorFormatSInt16:
    return "ds2::Architecture::kLLDBVectorFormatSInt16";
  case kLLDBVectorFormatUInt32:
    return "ds2::Architecture::kLLDBVectorFormatUInt32";
  case kLLDBVectorFormatSInt32:
    return "ds2::Architecture::kLLDBVectorFormatSInt32";
  case kLLDBVectorFormatUInt128:
    return "ds2::Architecture::kLLDBVectorFormatUInt128";
  case kLLDBVectorFormatFloat32:
    return "ds2::Architecture::kLLDBVectorFormatFloat32";
  default:
    break;
  }

  return "ds2::Architecture::kLLDBVectorFormatUInt8";
}

static void GenerateWarningMessage(FILE *fp, Context const &ctx) {
  fprintf(
      fp,
      "// !!! WARNING !!! THIS FILE WAS AUTOGENERATED !!! DO NOT MODIFY !!!\n"
      "// !!! WARNING !!! MODIFY INSTEAD: %s\n"
      "\n",
      ctx.Path.c_str());
}

static void GenerateProlog1(FILE *fp, Context const &ctx) {
  for (auto include : ctx.Includes) {
    fprintf(fp, "#include \"%s\"\n", include.c_str());
  }
  if (!ctx.Includes.empty()) {
    fprintf(fp, "\n");
  }

  fprintf(fp, "using ds2::Architecture::RegisterDef;\n");
  fprintf(fp, "using ds2::Architecture::FlagDef;\n");
  fprintf(fp, "using ds2::Architecture::FlagSet;\n");
  fprintf(fp, "using ds2::Architecture::GDBVectorDef;\n");
  fprintf(fp, "using ds2::Architecture::GDBVectorUnion;\n");
  fprintf(fp, "using ds2::Architecture::GDBVectorUnionField;\n");
  fprintf(fp, "using ds2::Architecture::GDBFeature;\n");
  fprintf(fp, "using ds2::Architecture::GDBFeatureEntry;\n");
  fprintf(fp, "using ds2::Architecture::LLDBRegisterSet;\n\n");

  fprintf(fp, "#if defined(ENDIAN_BIG)\n");
  fprintf(fp, "#define REG_REL_OFFSET(MAXBYTES, RELOFF, REGSIZE) ((MAXBYTES) - "
              "((RELOFF) + (REGSIZE))\n");
  fprintf(fp, "#else\n");
  fprintf(fp, "#define REG_REL_OFFSET(MAXBYTES, RELOFF, REGSIZE) (RELOFF)\n");
  fprintf(fp, "#endif\n\n");

  fprintf(fp, "namespace {\n\n");
}

static void GenerateEpilog1(FILE *fp, Context const &ctx) {
  fprintf(fp, "\n}\n\n");
}

static void GenerateProlog2(FILE *fp, Context const &ctx) {
  fprintf(fp, "//\n");
  fprintf(fp, "// Public Definitions\n");
  fprintf(fp, "//\n");
  fprintf(fp, "namespace ds2 { namespace Architecture { namespace %s {\n\n",
          ctx.Namespace.c_str());
}

static void GenerateEpilog2(FILE *fp, Context const &ctx) {
  fprintf(fp, "\n} } }\n");
}

static void GenerateForwardRegisterDecls(FILE *fp, Context const &ctx) {
  if (ctx.RegisterSets.empty())
    return;

  fprintf(fp, "//\n");
  fprintf(fp, "// Register Definitions (Forward Declarations)\n");
  fprintf(fp, "//\n\n");

  Register::set regs;

  for (auto regdef : ctx.RegisterSets) {
    if (!regs.empty()) {
      fprintf(fp, "\n");
    }

    fprintf(fp, "// Register Set (%s)\n", regdef.first.c_str());
    for (auto reg : *regdef.second) {
      fprintf(fp, "extern RegisterDef const reg_def_%s;\n", reg->CName.c_str());
      regs.insert(reg);
    }
  }

  fprintf(fp, "\n");
}

static void GenerateForwardFlagDecls(FILE *fp, Context const &ctx) {
  if (ctx.FlagSets.empty())
    return;

  fprintf(fp, "//\n");
  fprintf(fp, "// Flag Definitions (Forward Declarations)\n");
  fprintf(fp, "//\n\n");

  for (auto flagdef : ctx.FlagSets) {
    fprintf(fp, "extern FlagSet const flag_set_%s;\n", flagdef.first.c_str());
  }

  fprintf(fp, "\n");
}

static void GenerateForwardGDBVectorDecls(FILE *fp, Context const &ctx) {
  if (ctx.GDBVectorSet.empty())
    return;

  fprintf(fp, "//\n");
  fprintf(fp, "// GDB Vector Definitions (Forward Declarations)\n");
  fprintf(fp, "//\n\n");

  for (auto vecdef : ctx.GDBVectorSet) {
    if (vecdef->Encoding == kGDBEncodingUnion) {
      fprintf(fp, "extern GDBVectorUnion const vec_def_%s;\n",
              vecdef->Name.c_str());
    } else {
      fprintf(fp, "extern GDBVectorDef const vec_def_%s;\n",
              vecdef->Name.c_str());
    }
  }

  fprintf(fp, "\n");
}

static void GenerateForwardDecls(FILE *fp, Context const &ctx) {
  GenerateForwardRegisterDecls(fp, ctx);
  GenerateForwardFlagDecls(fp, ctx);
  GenerateForwardGDBVectorDecls(fp, ctx);
}

static void GenerateLLDBRegisterSets(FILE *fp, Context const &ctx) {
  if (ctx.LLDBDefs.empty())
    return;

  fprintf(fp, "//\n");
  fprintf(fp, "// LLDB Register Sets\n");
  fprintf(fp, "//\n");

  std::vector<std::string> names;

  for (auto def : ctx.LLDBDefs) {
    fprintf(fp, "\n");

    if (!def->Description.empty()) {
      fprintf(fp, "// LLDB Register Set (%s)\n\n", def->Description.c_str());
    } else {
      fprintf(fp, "// LLDB Register Set #%zu\n\n", def->Index);
    }

    std::string defname, setname;

    {
      std::stringstream ss;
      ss << "lldb_reg_defs_" << def->Index;
      defname = ss.str();
    }
    {
      std::stringstream ss;
      ss << "lldb_reg_set_" << def->Index;
      setname = ss.str();
    }

    fprintf(fp, "RegisterDef const * const %s[] = {\n", defname.c_str());

    size_t count = 0;
    for (auto regset : def->RegisterSets) {
      for (auto &reg : *regset) {
        fprintf(fp, "%*s&reg_def_%s,\n", 4, "", reg->CName.c_str());
        count++;
      }
    }
    fprintf(fp, "%*snullptr\n", 4, "");
    fprintf(fp, "};\n\n");

    fprintf(fp, "LLDBRegisterSet const %s = {\n", setname.c_str());
    if (!def->Description.empty()) {
      fprintf(fp, "%*s\"%s\",\n", 4, "", def->Description.c_str());
    } else {
      fprintf(fp, "%*snullptr,\n", 4, "");
    }
    fprintf(fp, "%*s%zu,\n", 4, "", count);
    fprintf(fp, "%*s%s\n", 4, "", defname.c_str());
    fprintf(fp, "};\n");

    names.push_back(setname);
  }

  fprintf(fp, "\n");
  fprintf(fp, "LLDBRegisterSet const * const lldb_reg_sets[] = {\n");
  for (auto name : names) {
    fprintf(fp, "%*s&%s,\n", 4, "", name.c_str());
  }
  fprintf(fp, "%*snullptr\n", 4, "");
  fprintf(fp, "};\n\n");
}

static void GenerateLLDBSubSets(FILE *fp, Context const &ctx) {
  if (!ctx.HasInvalidatedOrContainerSets)
    return;

  fprintf(fp, "//\n");
  fprintf(fp, "// LLDB Register Invalidation/Container Sets\n");
  fprintf(fp, "//\n\n");

  for (auto reg : ctx.Registers) {
    Register::set subset;

    if (!reg->InvalidateRegisters.empty()) {
      subset.clear();
      fprintf(fp, "RegisterDef const * const reg_inv_defs_%s[] = { ",
              reg->CName.c_str());
      for (auto sub : reg->InvalidateRegisters) {
        if (subset.find(sub) != subset.end())
          continue;

        fprintf(fp, "&reg_def_%s, ", sub->CName.c_str());
        subset.insert(sub);
      }
      fprintf(fp, "nullptr };\n");
    }

    if (!reg->ContainerRegisters.empty()) {
      subset.clear();
      fprintf(fp, "RegisterDef const * const reg_cnt_defs_%s[] = { ",
              reg->CName.c_str());
      for (auto sub : reg->ContainerRegisters) {
        if (subset.find(sub) != subset.end())
          continue;

        fprintf(fp, "&reg_def_%s, ", sub->CName.c_str());
        subset.insert(sub);
      }
      fprintf(fp, "nullptr };\n");
    }
  }

  fprintf(fp, "\n");
}

static void GenerateFlagDefs(FILE *fp, Context const &ctx) {
  if (ctx.FlagSets.empty())
    return;

  fprintf(fp, "//\n");
  fprintf(fp, "// Flag Sets\n");
  fprintf(fp, "//\n\n");

  size_t index = 0;
  for (auto fset : ctx.FlagSets) {
    if (index++ != 0) {
      fprintf(fp, "\n");
    }

    fprintf(fp, "// Flag Set (%s)\n", fset.first.c_str());

    fprintf(fp, "FlagDef const flag_defs_%s[] = {\n", fset.first.c_str());
    for (auto flag : *fset.second) {
      fprintf(fp, "%*s{ \"%s\", %zu, %zu },\n", 4, "", flag->Name.c_str(),
              flag->Start, flag->Length);
    }
    fprintf(fp, "%*s{ nullptr, 0, 0 }\n", 4, "");
    fprintf(fp, "};\n");

    fprintf(fp, "\n");
    fprintf(fp, "FlagSet const flag_set_%s = "
                "{ \"%s\", %zu, %zu, flag_defs_%s };\n",
            fset.first.c_str(), fset.second->GDBName().c_str(),
            fset.second->size(), fset.second->count(), fset.first.c_str());
  }

  fprintf(fp, "\n");
}

static void GenerateGDBVectorDefs(FILE *fp, Context const &ctx) {
  if (ctx.GDBVectorSet.empty())
    return;

  fprintf(fp, "//\n");
  fprintf(fp, "// GDB Vector Set\n");
  fprintf(fp, "//\n\n");

  for (auto vecdef : ctx.GDBVectorSet) {
    if (vecdef->Encoding == kGDBEncodingUnion) {
      fprintf(fp, "GDBVectorUnionField const vec_unf_%s[] = {\n",
              vecdef->Name.c_str());
      for (auto fld : vecdef->Union.FieldNames) {
        fprintf(fp, "%*s{ \"%s\", %s, %s },\n", 4, "", fld.Name.c_str(),
                NameForGDBEncoding(fld.Encoding),
                fld.Encoding == kGDBEncodingCustom
                    ? ("&vec_def_" + fld.EncodingName).c_str()
                    : "nullptr");
      }
      fprintf(
          fp,
          "%*s{ nullptr, ds2::Architecture::kGDBEncodingUnknown, nullptr }\n",
          4, "");
      fprintf(fp, "};\n");
      fprintf(fp, "GDBVectorUnion const vec_def_%s = { "
                  "\"%s\", %zu, vec_unf_%s };\n",
              vecdef->Name.c_str(), vecdef->Name.c_str(),
              vecdef->Union.FieldNames.size(), vecdef->Name.c_str());
    } else {
      fprintf(fp, "GDBVectorDef const vec_def_%s = { "
                  "\"%s\", %zu, %zd, %s "
                  "};\n",
              vecdef->Name.c_str(), vecdef->Name.c_str(), vecdef->BitSize,
              vecdef->ElementSize, NameForGDBEncoding(vecdef->Encoding));
    }
  }

  fprintf(fp, "\n");
}

static std::string MakeCName(std::string const &s) {
  if (s.empty())
    return std::string();

  std::string o;
  if (!(isalpha(s[0]) || s[0] == '_')) {
    o += '_';
  }

  for (size_t n = 0; n < s.length(); n++) {
    if (!(isalnum(s[n]) || s[n] == '_')) {
      o += '_';
    } else {
      o += s[n];
    }
  }

  return o;
}

static void GenerateGDBFeatures(FILE *fp, Context const &ctx) {
  if (ctx.GDBDefs.empty())
    return;

  fprintf(fp, "//\n");
  fprintf(fp, "// GDB Features\n");
  fprintf(fp, "//\n\n");

  std::vector<std::string> names;

  size_t index = 0;
  for (auto feat : ctx.GDBDefs.features()) {
    if (!feat->OSABI.empty() && ctx.SpecificOSABI != feat->OSABI)
      continue;

    if (index++ != 0) {
      fprintf(fp, "\n");
    }

    fprintf(fp, "// GDB Feature (%s)\n", feat->Identifier.c_str());

    size_t count = 0;
    std::string cname(MakeCName(feat->Identifier).c_str());
    if (!feat->Entries.empty()) {
      fprintf(fp, "GDBFeatureEntry const gdb_fents_%s[] = {\n", cname.c_str());
      for (auto e : feat->Entries) {
        if (e->Type == GDBFeatureEntry::kTypeFlagSet) {
          fprintf(
              fp,
              "%*s{ ds2::Architecture::kGDBFeatureTypeFlags, &flag_set_%s },\n",
              4, "", e->Set.Flag->name().c_str());
          count++;
        } else if (e->Type == GDBFeatureEntry::kTypeVectorSet) {
          fprintf(fp, "%*s{ ds2::Architecture::%s, &vec_def_%s },\n", 4, "",
                  e->Set.Vector->Encoding == kGDBEncodingUnion
                      ? "kGDBFeatureTypeUnion"
                      : "kGDBFeatureTypeVector",
                  e->Set.Vector->Name.c_str());
          count++;
        } else if (e->Type == GDBFeatureEntry::kTypeRegisterSet) {
          for (auto reg : *e->Set.Register) {
            fprintf(fp, "%*s{ ds2::Architecture::kGDBFeatureTypeRegister, "
                        "&reg_def_%s },\n",
                    4, "", reg->CName.c_str());
            count++;
          }
        }
      }
      fprintf(fp, "%*s{ ds2::Architecture::kGDBFeatureTypeNone, nullptr }\n", 4,
              "");
      fprintf(fp, "};\n");
    }

    std::string featname = "gdb_feat_" + cname;

    fprintf(fp, "GDBFeature const %s = "
                "{ \"%s\", %s, %zu, %s };\n",
            featname.c_str(), feat->Identifier.c_str(),
            QuoteOrNull(feat->FileName).c_str(), count,
            !feat->Entries.empty()
                ? ("gdb_fents_" + MakeCName(feat->Identifier)).c_str()
                : "nullptr");

    names.push_back(featname);
  }

  fprintf(fp, "\n");
  fprintf(fp, "// GDB Features (all)\n");
  fprintf(fp, "GDBFeature const * const gdb_features[] = {\n");
  for (auto cname : names) {
    fprintf(fp, "%*s&%s,\n", 4, "", cname.c_str());
  }
  fprintf(fp, "%*snullptr\n", 4, "");
  fprintf(fp, "};\n\n");
}

//
// This is an helper function to emit endian-neutral offset for
// subsetted registers.
//
static std::string GetLLDBOffset(Register const *parent, ssize_t bitsize,
                                 ssize_t offset) {
  std::ostringstream ss;
  if (parent != nullptr && !(bitsize < 0) && !(offset < 0)) {
    ss << parent->LLDBOffset << " + "
       << "REG_REL_OFFSET(" << (parent->BitSize >> 3) << ", " << offset << ", "
       << (bitsize >> 3) << ')';
  } else {
    ss << offset;
  }
  return ss.str();
}

static void GenerateRegisterDefs(FILE *fp, Context const &ctx) {
  fprintf(fp, "//\n");
  fprintf(fp, "// Register Definitions\n");
  fprintf(fp, "//\n\n");

  for (auto reg : ctx.Registers) {
    std::string invset, cntset;

    if (!reg->InvalidateRegisters.empty()) {
      invset = "reg_inv_defs_" + reg->CName;
    }

    if (!reg->ContainerRegisters.empty()) {
      cntset = "reg_cnt_defs_" + reg->CName;
    }

    std::string flags;
    if (reg->NoGDBRegisterNumber) {
      if (!flags.empty())
        flags += " | ";
      flags += "ds2::Architecture::kRegisterDefNoGDBRegisterNumber";
    }

    fprintf(fp,
            "RegisterDef const reg_def_%s = { "
            "%s, "  // reg->Name
            "%s, "  // reg->LLDBName
            "%s, "  // reg->AlternateName
            "%s, "  // reg->GenericName
            "%s, "  // reg->GDBGroup
            "%zd, " // reg->BitSize
            "%s, "  // reg->DWARFRegisterNumber
            "%s, "  // reg->GDBRegisterNumber
            "%s, "  // reg->EHFrameRegisterNumber
            "%s, "  // reg->LLDBRegisterNumber
            "%s, "  // reg->LLDBOffset
            "%s, "  // reg->LLDBVectorFormat
            "%s, "  // reg->Encoding
            "%s, "  // reg->Format
            "%s, "  // reg->GDBEncoding
            "%s, "  // reg->Flags
            "%s, "  // reg->InvalidateRegisters
            "%s "   // reg->ContainerRegisters
            "};\n",
            reg->CName.c_str(), QuoteOrNull(reg->Name).c_str(),
            QuoteOrNull(reg->LLDBName).c_str(),
            QuoteOrNull(reg->AlternateName).c_str(),
            QuoteOrNull(reg->GenericName).c_str(),
            QuoteOrNull(reg->GDBGroup).c_str(), reg->BitSize,
            NameOfRegister(reg->DWARFRegisterNumber, reg->Name,
                           "ds2::Architecture::" + ctx.Namespace +
                               "::reg_dwarf_").c_str(),
            NameOfRegister(reg->GDBRegisterNumber, reg->Name,
                           "ds2::Architecture::" + ctx.Namespace + "::reg_gdb_")
                .c_str(),
            NameOfRegister(reg->EHFrameRegisterNumber, reg->Name,
                           "ds2::Architecture::" + ctx.Namespace + "::reg_ehframe_")
                .c_str(),
            NameOfRegister(reg->LLDBRegisterNumber, reg->Name,
                           "ds2::Architecture::" + ctx.Namespace +
                               "::reg_lldb_").c_str(),
            GetLLDBOffset(reg->ParentRegister.get(), reg->BitSize,
                          reg->LLDBOffset).c_str(),
            NameForLLDBVectorFormat(reg->Format, reg->LLDBVectorFormat).c_str(),
            NameForEncoding(reg->Encoding), NameForFormat(reg->Format),
            BuildGDBEncodingArray(reg.get()).c_str(),
            flags.empty() ? "0" : flags.c_str(),
            invset.empty() ? "nullptr" : invset.c_str(),
            cntset.empty() ? "nullptr" : cntset.c_str());
  }
}

static void GenerateLLDBDescriptor(FILE *fp, Context const &ctx) {
  if (ctx.LLDBDefs.empty())
    return;

  fprintf(fp, "LLDBDescriptor const LLDB = { %zu, lldb_reg_sets };\n",
          ctx.LLDBDefs.count());
}

static void GenerateGDBDescriptor(FILE *fp, Context const &ctx) {
  if (ctx.GDBDefs.empty())
    return;

  fprintf(fp, "GDBDescriptor const GDB = { %s, %s, %zu, gdb_features };\n",
          QuoteOrNull(ctx.GDBDefs.architecture()).c_str(),
          QuoteOrNull(ctx.LongNameOSABI).c_str(),
          ctx.GDBDefs.features().size());
}

static void GenerateEnums(FILE *fp, Context const &ctx) {
  Register::vector regs;

  // DWARF
  regs.clear();
  for (auto reg : ctx.Registers) {
    if (!(reg->DWARFRegisterNumber < 0)) {
      regs.push_back(reg);
    }
  }

  if (!regs.empty()) {
    fprintf(fp, "enum /* dwarf_reg */ {\n");
    for (auto reg : regs) {
      fprintf(fp, "%*sreg_dwarf_%s = %zd,\n", 4, "", reg->Name.c_str(),
              reg->DWARFRegisterNumber);
    }
    fprintf(fp, "};\n\n");
  }

  // GDB
  regs.clear();
  for (auto reg : ctx.Registers) {
    if (!(reg->GDBRegisterNumber < 0)) {
      regs.push_back(reg);
    }
  }

  if (!regs.empty()) {
    fprintf(fp, "enum /* gdb_reg */ {\n");
    for (auto reg : regs) {
      fprintf(fp, "%*sreg_gdb_%s = %zd,\n", 4, "", reg->Name.c_str(),
              reg->GDBRegisterNumber);
    }
    fprintf(fp, "};\n\n");
  }

  // EHFrame
  regs.clear();
  for (auto reg : ctx.Registers) {
    if (!(reg->EHFrameRegisterNumber < 0)) {
      regs.push_back(reg);
    }
  }

  if (!regs.empty()) {
    fprintf(fp, "enum /* ehframe_reg */ {\n");
    for (auto reg : regs) {
      fprintf(fp, "%*sreg_ehframe_%s = %zd,\n", 4, "", reg->Name.c_str(),
              reg->EHFrameRegisterNumber);
    }
    fprintf(fp, "};\n\n");
  }

  // LLDB
  regs.clear();
  for (auto reg : ctx.Registers) {
    if (!(reg->LLDBRegisterNumber < 0)) {
      regs.push_back(reg);
    }
  }

  if (!regs.empty()) {
    fprintf(fp, "enum /* lldb_reg */ {\n");
    for (auto reg : regs) {
      fprintf(fp, "%*sreg_lldb_%s = %zd,\n", 4, "", reg->Name.c_str(),
              reg->LLDBRegisterNumber);
    }
    fprintf(fp, "};\n\n");
  }
}

static void GenerateHeaderProlog(FILE *fp, Context const &ctx) {
  fprintf(fp, "#pragma once\n\n");

  for (auto include : ctx.Includes) {
    fprintf(fp, "#include \"%s\"\n", include.c_str());
  }
  if (!ctx.Includes.empty()) {
    fprintf(fp, "\n");
  }

  fprintf(fp, "namespace ds2 { namespace Architecture { namespace %s {\n\n",
          ctx.Namespace.c_str());
}

static void GenerateHeaderEpilog(FILE *fp, Context const &ctx) {
  fprintf(fp, "} } }\n");
}

static void GenerateExterns(FILE *fp, Context const &ctx) {
  if (!ctx.LLDBDefs.empty()) {
    fprintf(fp, "extern LLDBDescriptor const LLDB;\n");
  }
  if (!ctx.GDBDefs.empty()) {
    fprintf(fp, "extern GDBDescriptor const GDB;\n");
  }
  if (!ctx.LLDBDefs.empty() || !ctx.GDBDefs.empty()) {
    fprintf(fp, "\n");
  }
}

static bool SetOSABI(Context &ctx, std::string const &osabi) {
  for (char const *const *koa = sKnownOSABI; *koa != nullptr; koa++) {
    if (*koa == osabi) {
      ctx.SpecificOSABI = osabi;
      ctx.GenericOSABI = GetGenericOSABIName(ctx.SpecificOSABI.c_str());
      ctx.LongNameOSABI = GetLongOSABIName(ctx.SpecificOSABI.c_str());
      return true;
    }
  }

  return false;
}

static void Help(char const *progname) {
  std::ostringstream skoa;
  for (char const *const *koa = sKnownOSABI; *koa != nullptr; koa++) {
    if (koa != sKnownOSABI) {
      skoa << ", ";
    }

    skoa << *koa;
  }

  fprintf(stderr, "usage: %s -a osabi -I include-file -c|-h [-f] -o "
                  "[filename|-] definition.json\n\n"
                  "switches:\n\n"
                  "\t-a osabi\tselect the target OS/ABI, valid values: %s\n"
                  "\t-I include-file\tadd an #include \"<include-file>\" to "
                  "the generated C++ file\n"
                  "\t-c\t\tgenerate the C++ source file (default)\n"
                  "\t-h\t\tgenerate the C++ header file\n"
                  "\t-o filename\tselects output destination, if '-' is "
                  "specified then use stdout\n"
                  "\t-f\t\toverwrite the output file if it exists\n",
          progname, skoa.str().c_str());

  exit(EXIT_FAILURE);
}

int main(int argc, char *const *argv) {
  char const *progname = *argv;
  char const *osabi = nullptr;
  char const *output = nullptr;
  bool header = false;
  bool overwrite = false;
  std::set<std::string> includes;

  Context ctx;

  int c;
  while ((c = getopt(argc, argv, "a:cfho:I:")) != EOF) {
    switch (c) {
    case 'a':
      osabi = optarg;
      break;
    case 'c':
      header = false;
      break;
    case 'h':
      header = true;
      break;
    case 'I':
      if (includes.find(optarg) == includes.end()) {
        ctx.Includes.push_back(optarg);
        includes.insert(optarg);
      }
      break;
    case 'o':
      output = optarg;
      break;
    case 'f':
      overwrite = true;
      break;
    default:
      Help(progname);
      exit(EXIT_FAILURE);
      break;
    }
  }

  argc -= optind;
  argv += optind;

  if (osabi == nullptr) {
    fprintf(stderr, "error: OS/ABI not specified\n");
    Help(progname);
    exit(EXIT_FAILURE);
  }

  if (argc < 1) {
    fprintf(stderr, "error: input file name not specified\n");
    Help(progname);
    exit(EXIT_FAILURE);
  }

  if (output == nullptr) {
    fprintf(stderr, "error: output file name not specified\n");
    Help(progname);
    exit(EXIT_FAILURE);
  }

  if (!SetOSABI(ctx, osabi)) {
    fprintf(stderr, "error: OS/ABI '%s' not supported\n", osabi);
    Help(progname);
    exit(EXIT_FAILURE);
  }

  FILE *fp = nullptr;
  if (strcmp(output, "-") == 0) {
    fp = stdout;
  } else {
    if (!overwrite && (access(output, R_OK) == 0 || errno != ENOENT)) {
      fprintf(stderr, "error: output file '%s' already exists, use -f if you "
                      "want overwrite\n",
              output);
      exit(EXIT_FAILURE);
    }
  }

  Parse(*argv, ctx);

  if (fp == nullptr) {
    fp = fopen(output, "wt");
    if (fp == nullptr) {
      fprintf(stderr, "error: cannot open output file '%s' for writing: %s\n",
              output, strerror(errno));
      exit(EXIT_FAILURE);
    }
  }

  GenerateWarningMessage(fp, ctx);

  if (header) {
    GenerateHeaderProlog(fp, ctx);
    GenerateEnums(fp, ctx);
    GenerateExterns(fp, ctx);
    GenerateHeaderEpilog(fp, ctx);
  } else {
    GenerateProlog1(fp, ctx);
    GenerateForwardDecls(fp, ctx);

    GenerateLLDBRegisterSets(fp, ctx);
    GenerateGDBFeatures(fp, ctx);

    GenerateLLDBSubSets(fp, ctx);

    GenerateFlagDefs(fp, ctx);
    GenerateGDBVectorDefs(fp, ctx);
    GenerateRegisterDefs(fp, ctx);
    GenerateEpilog1(fp, ctx);
    GenerateProlog2(fp, ctx);
    GenerateLLDBDescriptor(fp, ctx);
    GenerateGDBDescriptor(fp, ctx);
    GenerateEpilog2(fp, ctx);
  }

  if (fp != stdout) {
    fclose(fp);
  }

  return 0;
}
