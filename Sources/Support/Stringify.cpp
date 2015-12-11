//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Support/Stringify.h"
#include "DebugServer2/Utils/Log.h"

namespace ds2 {
namespace Support {

#define DO_STRINGIFY(VALUE)                                                    \
  case VALUE:                                                                  \
    return #VALUE;

#define DO_DEFAULT(MESSAGE, VALUE)                                             \
  default:                                                                     \
    if (dieOnFail)                                                             \
      DS2BUG(MESSAGE ": %#x", VALUE);                                          \
    else                                                                       \
      return nullptr;

char const *Stringify::Error(ErrorCode error, bool dieOnFail) {
  switch (error) {
    DO_STRINGIFY(kSuccess)
    DO_STRINGIFY(kErrorNoPermission)
    DO_STRINGIFY(kErrorNotFound)
    DO_STRINGIFY(kErrorProcessNotFound)
    DO_STRINGIFY(kErrorInterrupted)
    DO_STRINGIFY(kErrorInvalidHandle)
    DO_STRINGIFY(kErrorNoMemory)
    DO_STRINGIFY(kErrorAccessDenied)
    DO_STRINGIFY(kErrorInvalidAddress)
    DO_STRINGIFY(kErrorBusy)
    DO_STRINGIFY(kErrorAlreadyExist)
    DO_STRINGIFY(kErrorNoDevice)
    DO_STRINGIFY(kErrorNotDirectory)
    DO_STRINGIFY(kErrorIsDirectory)
    DO_STRINGIFY(kErrorInvalidArgument)
    DO_STRINGIFY(kErrorTooManySystemFiles)
    DO_STRINGIFY(kErrorTooManyFiles)
    DO_STRINGIFY(kErrorFileTooBig)
    DO_STRINGIFY(kErrorNoSpace)
    DO_STRINGIFY(kErrorInvalidSeek)
    DO_STRINGIFY(kErrorNotWriteable)
    DO_STRINGIFY(kErrorNameTooLong)
    DO_STRINGIFY(kErrorUnknown)
    DO_STRINGIFY(kErrorUnsupported)
    DO_DEFAULT("unknown error code", error);
  }
}
}
}
