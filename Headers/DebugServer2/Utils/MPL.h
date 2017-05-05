//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#pragma once

namespace ds2 {
namespace Utils {

template <typename Base, template <typename> class... Mixins> class MixinApply;

template <typename Base> class MixinApply<Base> : public Base {
public:
  template <typename... Args>
  explicit MixinApply(Args &&... args) : Base(std::forward<Args>(args)...) {}
};

template <typename Base, template <typename> class MixinHead,
          template <typename> class... MixinTail>
class MixinApply<Base, MixinHead, MixinTail...>
    : public MixinHead<MixinApply<Base, MixinTail...>> {
public:
  template <typename... Args>
  explicit MixinApply(Args &&... args)
      : MixinHead<MixinApply<Base, MixinTail...>>(std::forward<Args>(args)...) {
  }
};
} // namespace Utils
} // namespace ds2
