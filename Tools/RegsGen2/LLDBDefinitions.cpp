//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "LLDBDefinitions.h"
#include "Context.h"

LLDBDefinitions::LLDBDefinitions() {}

bool LLDBDefinitions::parse(Context &ctx, JSArray const *defs) {
  for (size_t index = 0; index < defs->count(); index++) {
    auto def = defs->value<JSDictionary>(index);
    if (def == nullptr) {
      fprintf(stderr, "warning: LLDB definition #%zu is not a "
                      "dictionary\n",
              index);
      continue;
    }

    auto sets = def->value<JSArray>("sets");
    if (sets == nullptr) {
      fprintf(stderr, "error: LLDB definition #%zu does not contain "
                      "mandatory 'sets' array\n",
              index);
      return false;
    }

    LLDBSet::shared_ptr set(new LLDBSet);

    set->Index = index;

    auto desc = def->value<JSString>("description");
    if (desc != nullptr) {
      set->Description = desc->value();
    }

    std::set<std::string> names;

    for (size_t n = 0; n < sets->count(); n++) {
      auto name = sets->value<JSString>(n);
      if (name == nullptr) {
        fprintf(stderr, "warning: LLDB definition #%zu set #%zu "
                        "does not specify a register set name.\n",
                index, n);
        continue;
      }

      if (names.find(name->value()) != names.end()) {
        fprintf(stderr, "warning: LLDB definition #%zu set #%zu "
                        "specifies register set '%s' more than once, "
                        "ignoring\n",
                index, n, name->value().c_str());
        continue;
      }

      auto regset = ctx.RegisterSets.find(name->value().c_str());
      if (regset == ctx.RegisterSets.end()) {
        fprintf(stderr, "error: LLDB definition #%zu set #%zu "
                        "specify a non-existent register set '%s'\n",
                index, n, name->value().c_str());
        return false;
      }

      set->RegisterSets.push_back(regset->second);
      names.insert(name->value());
    }

    _sets.push_back(set);
  }

  return true;
}
