// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

template <typename Enum> struct EnableBitMaskOperators {
  static const bool enable = false;
};

template <typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable,
                        Enum>::type constexpr
operator|(Enum lhs, Enum rhs) {
  using Underlying = typename std::underlying_type<Enum>::type;
  return static_cast<Enum>(static_cast<Underlying>(lhs) |
                           static_cast<Underlying>(rhs));
}

template <typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable,
                        Enum>::type constexpr
operator&(Enum lhs, Enum rhs) {
  using Underlying = typename std::underlying_type<Enum>::type;
  return static_cast<Enum>(static_cast<Underlying>(lhs) &
                           static_cast<Underlying>(rhs));
}

template <typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable,
                        Enum>::type constexpr
operator~(Enum e) {
  using Underlying = typename std::underlying_type<Enum>::type;
  return static_cast<Enum>(~static_cast<Underlying>(e));
}

template <typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable,
                        Enum>::type constexpr
operator!(Enum e) {
  using Underlying = typename std::underlying_type<Enum>::type;
  return static_cast<Enum>(!static_cast<Underlying>(e));
}

#define ENABLE_BITMASK_OPERATORS(x)                                            \
  template <> struct EnableBitMaskOperators<x> {                               \
    static const bool enable = true;                                           \
  }
