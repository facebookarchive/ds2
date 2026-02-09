// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#ifndef __regsgen_LLDBDefinitions_h
#define __regsgen_LLDBDefinitions_h

#include "RegisterSet.h"

struct LLDBSet {
  typedef std::shared_ptr<LLDBSet> shared_ptr;
  typedef std::vector<shared_ptr> vector;

  size_t Index;
  std::string Description;
  RegisterSet::vector RegisterSets;
};

class LLDBDefinitions {
private:
  LLDBSet::vector _sets;

public:
  LLDBDefinitions();

public:
  bool parse(Context &ctx, JSArray const *defs);

public:
  inline bool empty() const { return _sets.empty(); }

  inline size_t count() const { return _sets.size(); }

public:
  inline LLDBSet::vector::const_iterator begin() const { return _sets.begin(); }

  inline LLDBSet::vector::const_iterator end() const { return _sets.end(); }
};

#endif // !__regsgen_LLDBDefinitions_h
