//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __regsgen_ParseConstants_h
#define __regsgen_ParseConstants_h

#include "Constants.h"

#include <string>

bool ParseEncodingName(std::string const &name, Encoding &enc);
bool ParseGDBEncodingName(std::string const &name, GDBEncoding &enc,
                          std::string &cname);
bool ParseFormatName(std::string const &name, Format &fmt);
bool ParseLLDBVectorFormatName(std::string const &name, LLDBVectorFormat &fmt);

#endif // !__regsgen_ParseConstants_h
