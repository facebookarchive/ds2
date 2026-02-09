// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#pragma once

#include "DebugServer2/Types.h"

namespace ds2 {
namespace GDBRemote {

struct PacketProcessorDelegate;

class PacketProcessor {
public:
  virtual ~PacketProcessor() {}

protected:
  std::string _buffer;
  size_t _nreqs;
  bool _needhash;
  PacketProcessorDelegate *_delegate;

public:
  PacketProcessor();

public:
  inline void setDelegate(PacketProcessorDelegate *delegate) {
    _delegate = delegate;
  }
  inline PacketProcessorDelegate *delegate() const {
    return const_cast<PacketProcessor *>(this)->_delegate;
  }

public:
  void parse(std::string const &data);

private:
  void process();

private:
  bool validate();
};

struct PacketProcessorDelegate {
  virtual ~PacketProcessorDelegate() {}
  virtual void onPacketData(std::string const &data, bool valid) = 0;
  virtual void onInvalidData(std::string const &data) = 0;
};
} // namespace GDBRemote
} // namespace ds2
