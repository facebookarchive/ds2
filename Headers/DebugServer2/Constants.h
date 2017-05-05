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

//
// Endian
//

enum Endian {
  kEndianUnknown = 0,
  kEndianBig = 1,
  kEndianLittle = 2,
  kEndianPDP = 3,
#if defined(ENDIAN_BIG)
  kEndianNative = kEndianBig,
#elif defined(ENDIAN_LITTLE)
  kEndianNative = kEndianLittle,
#elif defined(ENDIAN_MIDDLE)
  kEndianNative = kEndianPDP,
#else
  kEndianNative = kEndianUnknown,
#endif
};

//
// Memory Protection
//

enum /*Protection*/ {
  kProtectionNone = 0,
  kProtectionExecute = (1 << 0),
  kProtectionWrite = (1 << 1),
  kProtectionRead = (1 << 2)
};

//
// Open flags
//

enum OpenFlags {
  kOpenFlagInvalid = 0,
  kOpenFlagRead = (1 << 0),
  kOpenFlagWrite = (1 << 1),
  kOpenFlagAppend = (1 << 2),
  kOpenFlagTruncate = (1 << 3),
  kOpenFlagNonBlocking = (1 << 4),
  kOpenFlagCreate = (1 << 5),
  kOpenFlagNewOnly = (1 << 6),
  kOpenFlagNoFollow = (1 << 7),
  kOpenFlagCloseOnExec = (1 << 8)
};
} // namespace ds2
