// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#ifndef __regsgen_GDBVectorSet_h
#define __regsgen_GDBVectorSet_h

#include "JSObjects/JSObjects.h"
#include "Definitions.h"

class GDBVectorSet {
public:
  typedef std::shared_ptr<GDBVectorSet> shared_ptr;

private:
  size_t _size;
  GDBVector::vector _vectors;
  GDBVector::name_map _map;

public:
  GDBVectorSet();

public:
  bool parse(JSDictionary const *d);

public:
  inline bool empty() const { return _vectors.empty(); }

public:
  inline GDBVector::vector::const_iterator begin() const {
    return _vectors.begin();
  }

  inline GDBVector::vector::const_iterator end() const {
    return _vectors.end();
  }
};

#endif // !__regsgen_GDBVectorSet_h
