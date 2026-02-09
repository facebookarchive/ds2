// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#ifndef __regsgen_RegisterSet_h
#define __regsgen_RegisterSet_h

#include "RegisterTemplate.h"

struct Context;

class RegisterSet {
public:
  typedef std::shared_ptr<RegisterSet> shared_ptr;
  typedef std::vector<shared_ptr> vector;
  typedef std::map<std::string, shared_ptr> name_map;

private:
  std::string _name;
  Register::vector _regs;
  Register::name_map _map;

public:
  RegisterSet();

public:
  bool parse(Context &ctx, std::string const &name, JSDictionary const *d);
  bool finalize(Context &ctx);

public:
  inline Register::vector::iterator begin() { return _regs.begin(); }

  inline Register::vector::iterator end() { return _regs.end(); }

public:
  inline Register::shared_ptr find(std::string const &name) const {
    auto ri = _map.find(name);
    if (ri == _map.end())
      return Register::shared_ptr();
    else
      return ri->second;
  }

public:
  inline std::string const &name() const { return _name; }
};

#endif // !__regsgen_RegisterSet_h
