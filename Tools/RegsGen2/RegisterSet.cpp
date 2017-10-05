//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "RegisterSet.h"
#include "Context.h"

RegisterSet::RegisterSet() {}

bool RegisterSet::parse(Context &ctx, std::string const &name,
                        JSDictionary const *d) {
  _name = name;

  //
  // Obtain and parse template
  //
  RegisterTemplate templat(ctx.SpecificOSABI, ctx.GenericOSABI);

  if (JSDictionary const *t = d->value<JSDictionary>("*")) {
    if (!templat.parse(t))
      return false;
  }

  //
  // Parse each register and add to the map.
  //
  for (auto rname : *d) {
    //
    // Skip template register
    //
    if (rname == "*")
      continue;

    if (_map.find(rname) != _map.end()) {
      fprintf(stderr, "error: register '%s' is already defined "
                      "for register set '%s'\n",
              rname.c_str(), _name.c_str());
      return false;
    }

    auto reg = templat.make(rname, d->value<JSDictionary>(rname));
    if (reg == nullptr)
      return false;

    //
    // Add self containing set to the referencing sets.
    //
    reg->ReferencingSets.insert(_name);

    _regs.push_back(Register::shared_ptr(reg));
    _map[rname] = _regs.back();
  }

  //
  // Now assign the GDB/EHFrame/DWARF register number, this must be done after
  // all registers are parsed to avoid overriding registers definining a
  // fixed register number.
  //
  for (auto reg : _regs) {
    templat.setRegisterNumbers(reg.get());
  }

  return true;
}

//
// Perform late validation and resolve referenced registers
// to complete LLDB invalidation and container sets.
//
bool RegisterSet::finalize(Context &ctx) {
  for (auto reg : _regs) {
    //
    // 1. Bit-size must be defined, and must be a power of two and
    //    greater than or equal to 8; for IEEE extended and vector
    //    types we check it is a multiple of 8.
    //
    if (reg->BitSize < 8) {
      fprintf(stderr, "error: register '%s' does specify a bit size that is "
                      "less than 8 bits\n",
              reg->Name.c_str());
      return false;
    }

    if (reg->Encoding != kEncodingIEEEExtended &&
        reg->Format != kFormatVector &&
        (reg->BitSize & (reg->BitSize - 1)) != 0) {
      fprintf(stderr, "error: register '%s' does specify a bit size "
                      "that is not a power of two\n",
              reg->Name.c_str());
      return false;
    }

    if ((reg->Encoding == kEncodingIEEEExtended || reg->Format == kFormatVector)
        && (reg->BitSize & 7) != 0) {
      fprintf(stderr, "error: register '%s' does specify a bit size "
                      "that is not a multiple of eight\n",
              reg->Name.c_str());
      return false;
    }

    //
    // 2. Warn if the register has no format or encoding.
    //
    if (reg->Format == kFormatInvalid) {
      fprintf(stderr, "warning: register '%s' does not specify "
                      "representation format\n",
              reg->Name.c_str());
    }

    if (reg->Encoding == kEncodingInvalid) {
      fprintf(stderr, "warning: register '%s' does not specify "
                      "encoding type\n",
              reg->Name.c_str());
    }

    //
    // 3. If the register is ieee-single, then bit size must be 32,
    //    if it's ieee-double then it must be 64, if it's
    //    ieee-extended it must be either 80, 96 or 128.
    //
    if (reg->Encoding == kEncodingIEEESingle && reg->BitSize != 32) {
      fprintf(stderr, "error: register '%s' is marked as IEEE single "
                      "but uses %zu bits while we were expecting 32\n",
              reg->Name.c_str(), reg->BitSize);
      return false;
    }

    if (reg->Encoding == kEncodingIEEEDouble && reg->BitSize != 64) {
      fprintf(stderr, "error: register '%s' is marked as IEEE double "
                      "but uses %zu bits while we were expecting 64\n",
              reg->Name.c_str(), reg->BitSize);
      return false;
    }

    if (reg->Encoding == kEncodingIEEEExtended &&
        !(reg->BitSize == 80 || reg->BitSize == 96 || reg->BitSize == 128)) {
      fprintf(stderr,
              "error: register '%s' is marked as IEEE extended "
              "but uses %zu bits, size must be either 80, 96 or 128 bits\n",
              reg->Name.c_str(), reg->BitSize);
      return false;
    }

    //
    // 4. Check that referencing sets exist, issue only warnings.
    //
    for (auto rsname : reg->ReferencingSets) {
      if (ctx.RegisterSets.find(rsname) == ctx.RegisterSets.end()) {
        fprintf(stderr, "warning: register '%s' references an "
                        "non-existent register set '%s'\n",
                reg->Name.c_str(), rsname.c_str());
      }
    }

    //
    // 5. Check and complete LLDB invalidation and container register sets;
    //    the referenced registers may be in any register set defined by
    //    ReferencingSets (and itself).
    //
    for (auto rname : reg->InvalidateRegisterNames) {
      bool found = false;

      for (auto rsname : reg->ReferencingSets) {
        auto rs = ctx.RegisterSets.find(rsname);
        if (rs == ctx.RegisterSets.end())
          continue;

        auto rreg = rs->second->find(rname);
        if (rreg == nullptr)
          continue;

        reg->InvalidateRegisters.push_back(rreg);
        found = true;
        break;
      }

      if (!found) {
        fprintf(stderr, "error: register '%s' references a "
                        "non-existent register '%s' used for invalidation\n",
                reg->Name.c_str(), rname.c_str());
        return false;
      }
    }

    for (auto rname : reg->ContainerRegisterNames) {
      bool found = false;

      for (auto rsname : reg->ReferencingSets) {
        auto rs = ctx.RegisterSets.find(rsname);
        if (rs == ctx.RegisterSets.end())
          continue;

        auto rreg = rs->second->find(rname);
        if (rreg == nullptr)
          continue;

        reg->ContainerRegisters.push_back(rreg);
        found = true;
        break;
      }

      if (!found) {
        fprintf(stderr, "error: register '%s' references a "
                        "non-existent register '%s' used as container\n",
                reg->Name.c_str(), rname.c_str());
        return false;
      }
    }

    //
    // 6. If GDB encoding is custom, then ensure that it is referencing
    //    a valid flag-set's item or vector-set's item.
    //
    if (reg->GDBEncoding == kGDBEncodingCustom) {
      bool valid = false;

      //
      // Try first flag sets.
      //
      for (auto flagset : ctx.FlagSets) {
        if (flagset.first == reg->GDBEncodingName ||
            flagset.second->GDBName() == reg->GDBEncodingName) {
          valid = true;
          break;
        }
      }

      if (!valid) {
        //
        // Try GDB vector set.
        //
        for (auto vec : ctx.GDBVectorSet) {
          if (vec->Name == reg->GDBEncodingName) {
            valid = true;
            break;
          }
        }
      }

      if (!valid) {
        fprintf(stderr, "error: register '%s' references a "
                        "non-existent GDB custom encoding '%s'\n",
                reg->Name.c_str(), reg->GDBEncodingName.c_str());
        return false;
      }
    }

    //
    // 7. Check parent for subsetting.
    //
    //    a. If Parent Set is set and Parent Element is non-negative,
    //       check the set exists.
    //    b. Check if Parent Register is set, if not check that
    //       Container Registers exists and contains only ONE
    //       element.
    //    c. Check that the parent register is part of the parent
    //       register set.
    //    d. Check that this register size * Parent Element still
    //       fits the parent register size.
    //    e. Assign the LLDBOffset to the relative offset (in bytes)
    //       and the pointer to the parent register.
    //
    if (!reg->ParentSetName.empty() && !(reg->ParentElement < 0)) {
      // (a)
      auto rset = ctx.RegisterSets.find(reg->ParentSetName);
      if (rset == ctx.RegisterSets.end()) {
        fprintf(stderr, "error: register '%s' references "
                        "'%s' as its parent register set, but it cannot be"
                        "found\n",
                reg->Name.c_str(), reg->ParentSetName.c_str());
        return false;
      }

      // (b)
      std::string pregname;
      if (!reg->ParentRegisterName.empty()) {
        pregname = reg->ParentRegisterName;
      } else if (!reg->ContainerRegisterNames.empty()) {
        if (reg->ContainerRegisterNames.size() > 1) {
          fprintf(stderr, "error: register '%s' references "
                          "'%s' as its parent register set, but it does not "
                          "make use of the parent-register key, it's not "
                          "possible to deduce the parent register from "
                          "container-registers because it contains multiple "
                          "entries\n",
                  reg->Name.c_str(), reg->ParentSetName.c_str());
          return false;
        }
        pregname = reg->ContainerRegisterNames[0];
        fprintf(stderr, "warning: register '%s' deduced '%s' as its "
                        "parent register from register set '%s', if it fails "
                        "please consider using the parent-register key\n",
                reg->Name.c_str(), pregname.c_str(),
                reg->ParentSetName.c_str());
      } else {
        fprintf(stderr, "error: register '%s' references parent set "
                        "'%s', but it's not possible to deduce the parent "
                        "register, please use the parent-register key\n",
                reg->Name.c_str(), reg->ParentSetName.c_str());
        return false;
      }

      // (c)
      Register::shared_ptr preg = rset->second->find(pregname);
      if (preg == nullptr) {
        fprintf(stderr, "error: register '%s' references register '%s' "
                        "as its parent register, but it couldn't be found in "
                        "register set '%s'\n",
                reg->Name.c_str(), pregname.c_str(),
                reg->ParentSetName.c_str());
        return false;
      }

      // (d)
      ssize_t bitoff = reg->BitSize * reg->ParentElement;
      if (bitoff + reg->BitSize > preg->BitSize) {
        fprintf(stderr, "error: register '%s' references element #%zd "
                        "of register '%s' in register set '%s', but given the "
                        "size of this register (%zd bits), and the parent "
                        "register (%zd bits), there can be at most %zd "
                        "elements\n",
                reg->Name.c_str(), reg->ParentElement, pregname.c_str(),
                reg->ParentSetName.c_str(), reg->BitSize, preg->BitSize,
                preg->BitSize / reg->BitSize);
        return false;
      }

      // (e)
      reg->LLDBOffset = bitoff >> 3;
      reg->ParentRegister = preg;
    }
  }

  return true;
}
