//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __regsgen_Context_h
#define __regsgen_Context_h

#include "RegisterSet.h"
#include "FlagSet.h"
#include "GDBVectorSet.h"
#include "LLDBDefinitions.h"
#include "GDBDefinitions.h"

struct Context {
  std::string Path;
  std::string SpecificOSABI;
  std::string GenericOSABI;
  std::string LongNameOSABI;
  std::string Namespace;
  std::vector<std::string> Includes;
  JSDictionary const *Root;
  FlagSet::name_map FlagSets;
  ::GDBVectorSet GDBVectorSet;
  Register::set Registers;
  Register::name_map PublicRegisters;
  RegisterSet::name_map RegisterSets;
  LLDBDefinitions LLDBDefs;
  GDBDefinitions GDBDefs;
  bool HasInvalidatedOrContainerSets;
};

#endif // !__regsgen_Context_h
