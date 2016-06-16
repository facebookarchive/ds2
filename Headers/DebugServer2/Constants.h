//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Constants_h
#define __DebugServer2_Constants_h

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
}

#endif // !__DebugServer2_Constants_h
