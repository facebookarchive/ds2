// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#pragma once

#include "DebugServer2/Types.h"

namespace ds2 {
namespace Host {

class Channel {
public:
  virtual ~Channel() = default;

public:
  virtual void close() = 0;

public:
  virtual bool connected() const = 0;

public:
  virtual bool wait(int ms = -1) = 0;

public:
  virtual ssize_t send(void const *buffer, size_t length) = 0;
  virtual ssize_t receive(void *buffer, size_t length) = 0;

public:
  virtual bool send(std::string const &buffer);
  virtual bool receive(std::string &buffer);
};
} // namespace Host
} // namespace ds2
