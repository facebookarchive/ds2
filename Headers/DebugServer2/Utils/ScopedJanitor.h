//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

// This is a dummy implementation of the ScopeGuard idiom. We don't care about
// exceptions here, we just want to run some cleanup code at scope exit.

#pragma once

namespace ds2 {
namespace Utils {

template <typename Callable> class ScopedJanitor {
public:
  ScopedJanitor(Callable &&c) : _c(std::forward<Callable>(c)) {}
  ~ScopedJanitor() { _c(); }

  ScopedJanitor(ScopedJanitor &&rhs) : _c(std::move(rhs._c)) {}

  ScopedJanitor(ScopedJanitor const &rhs) = delete;
  ScopedJanitor &operator=(ScopedJanitor const &rhs) = delete;
  ScopedJanitor &operator=(ScopedJanitor &&rhs) = delete;

protected:
  Callable _c;
};

template <typename Callable> ScopedJanitor<Callable> MakeJanitor(Callable &&c) {
  return ScopedJanitor<Callable>(std::forward<Callable>(c));
}
}
}
