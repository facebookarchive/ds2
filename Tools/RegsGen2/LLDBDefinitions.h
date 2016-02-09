//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

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
