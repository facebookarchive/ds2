//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __regsgen_FlagSet_h
#define __regsgen_FlagSet_h

#include "JSObjects/JSObjects.h"
#include "Definitions.h"

#include <sys/types.h>

class FlagSet {
public:
  typedef std::shared_ptr<FlagSet> shared_ptr;
  typedef std::map<std::string, shared_ptr> name_map;

private:
  ssize_t _size;
  std::string _name;
  std::string _GDBName;
  Flag::vector _flags;

public:
  FlagSet();

public:
  bool parse(std::string const &name, JSDictionary const *d);

public:
  inline std::string const &name() const { return _name; }

  inline std::string const &GDBName() const { return _GDBName; }

public:
  inline size_t size() const { return _size; }

public:
  inline bool empty() const { return _flags.empty(); }

  inline size_t count() const { return _flags.size(); }

public:
  inline Flag::vector::const_iterator begin() const { return _flags.begin(); }

  inline Flag::vector::const_iterator end() const { return _flags.end(); }
};

#endif // !__regsgen_FlagSet_h
