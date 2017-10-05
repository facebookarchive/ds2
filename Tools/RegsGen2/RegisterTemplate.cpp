//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "RegisterTemplate.h"
#include "ParseConstants.h"

RegisterTemplate::Number::Number() : _base(-1), _next(0) {}

void RegisterTemplate::Number::init(size_t base) { _base = base; }

bool RegisterTemplate::Number::mark(size_t index) {
  if (_base < 0 || static_cast<ssize_t>(index) < _base)
    return false;

  index -= _base;
  if (_used.size() <= index) {
    _used.resize(index + 1);
  }

  if (_used[index])
    return false;

  _used[index] = true;
  return true;
}

ssize_t RegisterTemplate::Number::next() {
  if (_base < 0)
    return -1;

  if (_used.size() <= _next) {
    _used.resize(_next + 1);
  }

  while (_used[_next] && _next < _used.size()) {
    _next++;
  }

  _used[_next] = true;
  ssize_t ret = _base + _next;
  _next++;
  return ret;
}

RegisterTemplate::RegisterTemplate(std::string const &specificOSABI,
                                   std::string const &genericOSABI)
    : _DWARFEhframeAliased(false), _explicitGDBRegisterNumber(false),
      _specificOSABI(specificOSABI), _genericOSABI(genericOSABI) {}

//
// Register template may contain following keys:
//
// bit-size
// encoding
// gdb-encoding
// format
// lldb-vector-format
//
// base-dwarf-reg-number
// base-gdb-reg-number
// base-ehframe-reg-number
// explicit-gdb-number -- when generating GDB tables, emit only the reg number
//                        where specified.
//
// dwarf-ehframe-alias (DWARF and EHFrame register numbers are the same)
//
// referencing-sets
//
// private    -- the register is not shared globally, most of the time set
//               to true in register sets used to generate LLDB invalidation
//               and container registers
//
// gdb-group
// no-gdb-reg-number -- tell the XML generator to NOT include the regnum field
//
// these two keys are used to compute relative register offsets for LLDB:
// parent-set
// parent-element
//
// the register shall define a register in container-registers or
// parent-register, this is also true in the case container-registers has
// multiple registers.
//

bool RegisterTemplate::parse(JSDictionary const *d) {
  if (auto bitsize = d->value<JSInteger>("bit-size")) {
    _template.BitSize = bitsize->value();
  }

  if (auto encoding = d->value<JSString>("encoding")) {
    if (!ParseEncodingName(encoding->value(), _template.Encoding)) {
      fprintf(stderr, "error: encoding '%s' is not recognized\n",
              encoding->value().c_str());
      return false;
    }
  }

  if (auto GDBEncoding = d->value<JSString>("gdb-encoding")) {
    if (!ParseGDBEncodingName(GDBEncoding->value(), _template.GDBEncoding,
                              _template.GDBEncodingName)) {
      fprintf(stderr, "error: GDB encoding '%s' is not valid\n",
              GDBEncoding->value().c_str());
      return false;
    }
  }

  if (auto format = d->value<JSString>("format")) {
    if (!ParseFormatName(format->value(), _template.Format)) {
      fprintf(stderr, "error: format '%s' is not recognized\n",
              format->value().c_str());
      return false;
    }
  }

  if (auto LLDBVectorFormat = d->value<JSString>("lldb-vector-format")) {
    if (!ParseLLDBVectorFormatName(LLDBVectorFormat->value(),
                                   _template.LLDBVectorFormat)) {
      fprintf(stderr, "error: LLDB vector format '%s' is not recognized\n",
              LLDBVectorFormat->value().c_str());
      return false;
    }
  }

  if (auto base = d->value<JSInteger>("base-dwarf-reg-number")) {
    if (base->value() < 0) {
      fprintf(stderr, "error: base DWARF register number cannot be "
                      "negative\n");
      return false;
    }

    _DWARFRegisterNumber.init(base->value());
  }

  if (auto base = d->value<JSInteger>("base-gdb-reg-number")) {
    if (base->value() < 0) {
      fprintf(stderr, "error: base GDB register number cannot be "
                      "negative\n");
      return false;
    }

    _GDBRegisterNumber.init(base->value());
  }

  if (auto explct = d->value<JSBoolean>("explicit-gdb-reg-number")) {
    _explicitGDBRegisterNumber = explct;
  }

  if (auto base = d->value<JSInteger>("base-ehframe-reg-number")) {
    if (base->value() < 0) {
      fprintf(stderr, "error: base EHFrame register number cannot be "
                      "negative\n");
      return false;
    }

    if (auto alias = d->value<JSBoolean>("dwarf-ehframe-alias")) {
      if (alias->value()) {
        fprintf(stderr, "warning: defining EHFrame to be alias of DWARF "
                        "registers and setting base EHFrame register at the "
                        "same time\n");
      }
    }

    _ehframeRegisterNumber.init(base->value());
  } else if (auto alias = d->value<JSBoolean>("dwarf-ehframe-alias")) {
    _DWARFEhframeAliased = alias->value();
  }

  if (auto priv = d->value<JSBoolean>("private")) {
    _template.Private = priv->value();
  }

  if (auto ngrn = d->value<JSBoolean>("no-gdb-reg-number")) {
    _template.NoGDBRegisterNumber = ngrn->value();
  }

  if (auto refsets = d->value<JSArray>("referencing-sets")) {
    for (size_t n = 0; n < refsets->count(); n++) {
      if (auto name = refsets->value<JSString>(n)) {
        if (!name->value().empty()) {
          _template.ReferencingSets.insert(name->value());
        }
      }
    }
  }

  if (auto group = d->value<JSString>("gdb-group")) {
    if (group->value().empty()) {
      fprintf(stderr, "error: default GDB group name cannot be empty\n");
      return false;
    }

    _template.GDBGroup = group->value();
  }

  if (auto parentSet = d->value<JSString>("parent-set")) {
    if (parentSet->value().empty()) {
      fprintf(stderr, "error: default parent set name cannot be empty\n");
      return false;
    }

    _template.ParentSetName = parentSet->value();
  }

  if (auto parentElement = d->value<JSInteger>("parent-element")) {
    if (parentElement->value() < 0) {
      fprintf(stderr, "error: default parent element cannot be negative\n");
      return false;
    }

    _template.ParentElement = parentElement->value();
  }

  return true;
}

Register *RegisterTemplate::make(std::string const &name,
                                 JSDictionary const *d) {
  if (name.empty())
    return nullptr;

  //
  // Create a copy of the template
  //
  std::unique_ptr<Register> reg(new Register(_template));

  reg->Name = name;

  //
  // Parse register-specific keys
  //
  if (auto LLDBName = d->value<JSString>("lldb-name")) {
    if (LLDBName->value().empty()) {
      fprintf(stderr, "error: register '%s' specifies an empty LLDB "
                      "register name\n",
              name.c_str());
      return nullptr;
    }

    reg->LLDBName = LLDBName->value();
  }

  if (auto altName = d->value<JSString>("alternate-name")) {
    if (altName->value().empty()) {
      fprintf(stderr, "error: register '%s' specifies an empty "
                      "alternate name\n",
              name.c_str());
      return nullptr;
    }

    reg->AlternateName = altName->value();
  } else if (auto altNameDict = d->value<JSDictionary>("alternate-name")) {
    auto altName = altNameDict->value<JSString>(_specificOSABI);
    if (altName == nullptr) {
      altName = altNameDict->value<JSString>(_genericOSABI);
    }
    if (altName != nullptr) {
      if (altName->value().empty()) {
        fprintf(stderr, "error: register '%s' specifies an empty "
                        "alternate name\n",
                name.c_str());
        return nullptr;
      }

      reg->AlternateName = altName->value();
    } else {
      // Check negated keys
      for (auto key : *altNameDict) {
        if (key[0] == '!' && key.substr(1) != _specificOSABI) {
          auto altName = altNameDict->value<JSString>(key);
          if (altName->value().empty()) {
            fprintf(stderr, "error: register '%s' specifies an empty "
                            "alternate name\n",
                    name.c_str());
            return nullptr;
          }

          reg->AlternateName = altName->value();
          break;
        }
      }
    }
  }

  if (auto genericName = d->value<JSString>("generic-name")) {
    if (genericName->value().empty()) {
      fprintf(stderr, "error: register '%s' specifies an empty "
                      "generic name\n",
              name.c_str());
      return nullptr;
    }

    reg->GenericName = genericName->value();
  } else if (auto genericNameDict = d->value<JSDictionary>("generic-name")) {
    auto genericName = genericNameDict->value<JSString>(_specificOSABI);
    if (genericName == nullptr) {
      genericName = genericNameDict->value<JSString>(_genericOSABI);
    }
    if (genericName != nullptr) {
      if (genericName->value().empty()) {
        fprintf(stderr, "error: register '%s' specifies an empty "
                        "generic name\n",
                name.c_str());
        return nullptr;
      }

      reg->GenericName = genericName->value();
    } else {
      // Check negated keys
      for (auto key : *genericNameDict) {
        if (key[0] == '!' && key.substr(1) != _specificOSABI) {
          auto genericName = genericNameDict->value<JSString>(key);
          if (genericName->value().empty()) {
            fprintf(stderr, "error: register '%s' specifies an "
                            "empty generic name\n",
                    name.c_str());
            return nullptr;
          }

          reg->GenericName = genericName->value();
          break;
        }
      }
    }
  }

  if (auto base = d->value<JSInteger>("dwarf-reg-number")) {
    if (base->value() < 0) {
      fprintf(stderr, "error: register '%s' specifies a negative "
                      "DWARF register number\n",
              name.c_str());
      return nullptr;
    }

    _DWARFRegisterNumber.mark(base->value());
    reg->DWARFRegisterNumber = base->value();

    if (_DWARFEhframeAliased) {
      reg->EHFrameRegisterNumber = base->value();
    }
  }

  if (auto base = d->value<JSInteger>("gdb-reg-number")) {
    if (base->value() < 0) {
      fprintf(stderr, "error: register '%s' specifies a negative "
                      "GDB register number\n",
              name.c_str());
      return nullptr;
    }

    _GDBRegisterNumber.mark(base->value());
    reg->GDBRegisterNumber = base->value();
  }

  if (!_DWARFEhframeAliased) {
    if (auto base = d->value<JSInteger>("ehframe-reg-number")) {
      if (base->value() < 0) {
        fprintf(stderr, "error: register '%s' specifies a negative "
                        "EHFrame register number",
                name.c_str());
        return nullptr;
      }

      _ehframeRegisterNumber.mark(base->value());
      reg->EHFrameRegisterNumber = base->value();
    }
  }

  if (auto invregs = d->value<JSArray>("invalidate-registers")) {
    for (size_t n = 0; n < invregs->count(); n++) {
      if (auto name = invregs->value<JSString>(n)) {
        if (!name->value().empty()) {
          reg->InvalidateRegisterNames.push_back(name->value());
        }
      }
    }
  }

  if (auto invregs = d->value<JSArray>("container-registers")) {
    for (size_t n = 0; n < invregs->count(); n++) {
      if (auto name = invregs->value<JSString>(n)) {
        if (!name->value().empty()) {
          reg->ContainerRegisterNames.push_back(name->value());
        }
      }
    }
  }

  //
  // Parse all the fields overriding the template.
  //
  if (auto bitsize = d->value<JSInteger>("bit-size")) {
    reg->BitSize = bitsize->value();
  }

  if (auto encoding = d->value<JSString>("encoding")) {
    if (!ParseEncodingName(encoding->value(), reg->Encoding)) {
      fprintf(stderr, "error: register '%s' specifies an unrecognized "
                      "encoding '%s'\n",
              name.c_str(), encoding->value().c_str());
      return nullptr;
    }
  }

  if (auto GDBEncoding = d->value<JSString>("gdb-encoding")) {
    if (!ParseGDBEncodingName(GDBEncoding->value(), reg->GDBEncoding,
                              reg->GDBEncodingName)) {
      fprintf(stderr, "error: register '%s' specifies an unrecognized "
                      "GDB encoding '%s'\n",
              name.c_str(), GDBEncoding->value().c_str());
      return nullptr;
    }

    if (reg->GDBEncoding == kGDBEncodingUnion) {
      fprintf(stderr, "error: register '%s' tries to use GDB union "
                      "encoding, which is not supported for registers\n",
              name.c_str());
      return nullptr;
    }
  }

  if (auto format = d->value<JSString>("format")) {
    if (!ParseFormatName(format->value(), reg->Format)) {
      fprintf(stderr, "error: register '%s' specifies an unrecognized "
                      "format '%s'\n",
              name.c_str(), format->value().c_str());
      return nullptr;
    }
  }

  if (auto LLDBVectorFormat = d->value<JSString>("lldb-vector-format")) {
    if (!ParseLLDBVectorFormatName(LLDBVectorFormat->value(),
                                   reg->LLDBVectorFormat)) {
      fprintf(stderr, "error: register '%s' specifies an LLDB vector "
                      "format '%s' that is not recognized\n",
              name.c_str(), LLDBVectorFormat->value().c_str());
      return nullptr;
    }
  }

  if (auto refsets = d->value<JSArray>("referencing-sets")) {
    for (size_t n = 0; n < refsets->count(); n++) {
      if (auto name = refsets->value<JSString>(n)) {
        if (!name->value().empty()) {
          reg->ReferencingSets.insert(name->value());
        }
      }
    }
  }

  if (auto priv = d->value<JSBoolean>("private")) {
    reg->Private = priv->value();
  }

  if (auto group = d->value<JSString>("gdb-group")) {
    //
    // The gdb-group can be empty here to override the template.
    //
    reg->GDBGroup = group->value();
  }

  if (auto parentSet = d->value<JSString>("parent-set")) {
    // Allow empty name to override template.
    reg->ParentSetName = parentSet->value();
  }

  if (auto parentElement = d->value<JSInteger>("parent-element")) {
    // Allow negative values to override template.
    reg->ParentElement = parentElement->value();
  }

  if (auto parentReg = d->value<JSString>("parent-register")) {
    // Allow empty name to override template.
    reg->ParentRegisterName = parentReg->value();
  }

  return reg.release();
}

//
// This must be called once all registers are created to avoid
// collisions with fixed register numbers.
//
void RegisterTemplate::setRegisterNumbers(Register *reg) {
  //
  // Now generate the register numbers.
  //
  if (reg->DWARFRegisterNumber < 0) {
    reg->DWARFRegisterNumber = _DWARFRegisterNumber.next();
  }
  if (_DWARFEhframeAliased) {
    reg->EHFrameRegisterNumber = reg->DWARFRegisterNumber;
  } else if (reg->EHFrameRegisterNumber < 0) {
    reg->EHFrameRegisterNumber = _ehframeRegisterNumber.next();
  }
  if (!_explicitGDBRegisterNumber && reg->GDBRegisterNumber < 0) {
    reg->GDBRegisterNumber = _GDBRegisterNumber.next();
  }
}
