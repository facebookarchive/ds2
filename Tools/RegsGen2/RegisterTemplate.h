//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __regsgen_RegisterTemplate_h
#define __regsgen_RegisterTemplate_h

#include "JSObjects/JSObjects.h"
#include "Definitions.h"

class RegisterTemplate {
private:
  class Number {
  private:
    ssize_t _base;
    std::vector<bool> _used;
    size_t _next;

  public:
    Number();

  public:
    void init(size_t base);

  public:
    bool mark(size_t index);

  public:
    ssize_t next();
  };

private:
  Register _template;
  Number _DWARFRegisterNumber;
  Number _ehframeRegisterNumber;
  Number _GDBRegisterNumber;
  bool _DWARFEhframeAliased;
  bool _explicitGDBRegisterNumber;
  std::string _specificOSABI;
  std::string _genericOSABI;

public:
  RegisterTemplate(std::string const &specificOSABI = std::string(),
                   std::string const &genericOSABI = std::string());

public:
  bool parse(JSDictionary const *d);

public:
  //
  // Create a register out of the template.
  //
  Register *make(std::string const &name, JSDictionary const *d);

  //
  // Assign the register numbers to the register, call this once
  // you have created all the registers in the same creation order.
  //
  void setRegisterNumbers(Register *reg);
};

#endif // !__regsgen_RegisterTemplate_h
