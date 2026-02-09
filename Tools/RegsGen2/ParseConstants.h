// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

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
