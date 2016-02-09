//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __regsgen_GDBDefinitions_h
#define __regsgen_GDBDefinitions_h

#include "FlagSet.h"
#include "RegisterSet.h"
#include "GDBVectorSet.h"

struct GDBFeatureEntry {
  typedef std::shared_ptr<GDBFeatureEntry> shared_ptr;
  typedef std::vector<shared_ptr> vector;

  enum Type { kTypeNone, kTypeFlagSet, kTypeVectorSet, kTypeRegisterSet };

  GDBFeatureEntry::Type Type;

  struct {
    FlagSet::shared_ptr Flag;
    GDBVector::shared_ptr Vector;
    RegisterSet::shared_ptr Register;
  } Set;

  GDBFeatureEntry() : Type(kTypeNone) {}
};

struct GDBFeature {
  typedef std::shared_ptr<GDBFeature> shared_ptr;
  typedef std::vector<shared_ptr> vector;

  std::string FileName;
  std::string Identifier;
  std::string OSABI;

  GDBFeatureEntry::vector Entries;
};

class GDBDefinitions {
private:
  std::string _architecture;
  GDBFeature::vector _features;

public:
  GDBDefinitions();

public:
  bool parse(Context &ctx, JSDictionary const *d);

private:
  bool parseFeature(Context &ctx, size_t index, JSDictionary const *d);

public:
  inline bool empty() const { return _features.empty(); }

public:
  inline std::string const &architecture() const { return _architecture; }

public:
  inline GDBFeature::vector const &features() const { return _features; }
};

#endif // !__regsgen_GDBDefinitions_h
