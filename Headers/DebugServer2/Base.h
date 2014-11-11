//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Base_h
#define __DebugServer2_Base_h

#ifdef _WIN32
#include <WinSock2.h>
typedef SSIZE_T ssize_t;
#endif

#include <sys/types.h>
#include <stdint.h>

#include <set>
#include <vector>
#include <map>
#include <string>
#include <functional>

#include <cstring>

template <typename TYPE, size_t SIZE>
static inline size_t array_size(TYPE const (&)[SIZE]) {
  return SIZE;
}

#endif // !__DebugServer2_Base_h
