//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

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
