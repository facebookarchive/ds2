//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "GDBDefinitions.h"
#include "Context.h"

GDBDefinitions::GDBDefinitions() {}

bool GDBDefinitions::parse(Context &ctx, JSDictionary const *d) {
  if (auto arch = d->value<JSString>("architecture")) {
    _architecture = arch->value();
  } else {
    fprintf(stderr, "error: GDB definitions dictionary does not "
                    "specify target GDB architecture\n");
    return false;
  }

  if (auto feats = d->value<JSArray>("features")) {
    for (size_t n = 0; n < feats->count(); n++) {
      auto feat = feats->value<JSDictionary>(n);
      if (feat == nullptr) {
        fprintf(stderr, "error: GDB feature definition #%zu "
                        "does not specify a dictionary\n",
                n);
        return false;
      }

      if (!parseFeature(ctx, n, feat))
        return false;
    }
  }

  return true;
}

bool GDBDefinitions::parseFeature(Context &ctx, size_t index,
                                  JSDictionary const *d) {
  auto ident = d->value<JSString>("identifier");
  if (ident == nullptr) {
    fprintf(stderr, "error: GDB feature definition #%zu does "
                    "not specify mandatory identifier key\n",
            index);
    return false;
  }

  auto filename = d->value<JSString>("filename");
  auto contents = d->value<JSArray>("contents");
  auto osabi = d->value<JSString>("osabi");

  GDBFeature::shared_ptr feat(new GDBFeature);

  feat->Identifier = ident->value();
  if (filename != nullptr) {
    feat->FileName = filename->value();
  }
  if (osabi != nullptr) {
    feat->OSABI = osabi->value();
  }

  if (contents != nullptr) {
    for (size_t n = 0; n < contents->count(); n++) {
      auto typval = contents->value<JSString>(n);
      if (typval == nullptr) {
        fprintf(stderr, "error: GDB feature definition #%zu "
                        "for entry #%zu does not specify a valid "
                        "type\n",
                index, n);
        return false;
      }

      std::string type, name;

      type = typval->value();
      size_t namepos = type.find(':');
      if (namepos == std::string::npos || namepos + 1 >= type.length()) {
        fprintf(stderr, "error: GDB feature definition #%zu "
                        "for entry #%zu specifies an invalid reference '%s', "
                        "the format is set-type:set-name\n",
                index, n, type.c_str());
        return false;
      }
      name = type.substr(namepos + 1);
      type = type.substr(0, namepos);

      if (type == "flag-sets") {
        auto flagset = ctx.FlagSets.find(name);
        if (flagset == ctx.FlagSets.end()) {
          fprintf(stderr, "error: GDB feature definition #%zu "
                          "specifies unknown flag set '%s'\n",
                  index, name.c_str());
          return false;
        } else {
          GDBFeatureEntry::shared_ptr fe(new GDBFeatureEntry);
          fe->Type = GDBFeatureEntry::kTypeFlagSet;
          fe->Set.Flag = flagset->second;
          feat->Entries.push_back(fe);
        }
      } else if (type == "gdb-vector-set") {
        bool found = false;
        for (auto vec : ctx.GDBVectorSet) {
          if (vec->Name == name) {
            GDBFeatureEntry::shared_ptr fe(new GDBFeatureEntry);
            fe->Type = GDBFeatureEntry::kTypeVectorSet;
            fe->Set.Vector = vec;
            feat->Entries.push_back(fe);
            found = true;
            break;
          }
        }

        if (!found) {
          fprintf(stderr, "error: GDB feature definition #%zu "
                          "specifies unknown vector set '%s'\n",
                  index, name.c_str());
          return false;
        }
      } else if (type == "register-sets") {
        auto regset = ctx.RegisterSets.find(name);
        if (regset == ctx.RegisterSets.end()) {
          fprintf(stderr, "error: GDB feature definition #%zu "
                          "specifies unknown register set '%s'\n",
                  index, name.c_str());
          return false;
        } else {
          GDBFeatureEntry::shared_ptr fe(new GDBFeatureEntry);
          fe->Type = GDBFeatureEntry::kTypeRegisterSet;
          fe->Set.Register = regset->second;
          feat->Entries.push_back(fe);
        }
      } else {
        fprintf(stderr, "error: GDB feature definition #%zu "
                        "for entry '%s' specifies an unknown type '%s'\n",
                index, name.c_str(), type.c_str());
      }
    }
  }

  _features.push_back(feat);

  return true;
}
