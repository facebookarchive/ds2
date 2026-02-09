// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#pragma once

namespace ds2 {

static constexpr uint16_t Swap16(uint16_t x) { return (x >> 8) | (x << 8); }

static constexpr uint32_t Swap32(uint32_t x) {
  return Swap16(x >> 16) | (Swap16(x & 0xffff) << 16);
}

static constexpr uint64_t Swap64(uint64_t x) {
  return Swap32(x >> 32) |
         (static_cast<uint64_t>(Swap32(x & 0xffffffff)) << 32);
}
} // namespace ds2
