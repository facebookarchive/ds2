//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __regsgen_KeyValue_h
#define __regsgen_KeyValue_h

#include <string>

template <typename V> struct KeyValue {
  char const *Key;
  V Value;
};

template <typename V>
static inline bool FindKeyValue(KeyValue<V> const *kv, std::string const &key,
                                V &result) {
  for (size_t n = 0; kv[n].Key != nullptr; n++) {
    if (key == kv[n].Key) {
      result = kv[n].Value;
      return true;
    }
  }
  return false;
}

#endif // !__regsgen_KeyValue_h
