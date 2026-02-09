// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the Apache License v2.0 with LLVM
// Exceptions found in the LICENSE file in the root directory of this
// source tree.

#include "DebugServer2/Core/ErrorCodes.h"

namespace ds2 {

char const *GetErrorCodeString(ErrorCode err) {
  switch (err) {
  case kSuccess:
    return "success";
  case kErrorNoPermission:
    return "no permission";
  case kErrorNotFound:
    return "file not found";
  case kErrorProcessNotFound:
    return "process not found";
  case kErrorInterrupted:
    return "interrupted";
  case kErrorInvalidHandle:
    return "invalid handle";
  case kErrorNoMemory:
    return "no memory";
  case kErrorAccessDenied:
    return "access denied";
  case kErrorInvalidAddress:
    return "invalid address";
  case kErrorBusy:
    return "busy";
  case kErrorAlreadyExist:
    return "file already exists";
  case kErrorNoDevice:
    return "no device";
  case kErrorNotDirectory:
    return "not a directory";
  case kErrorIsDirectory:
    return "is a directory";
  case kErrorInvalidArgument:
    return "invalid argument";
  case kErrorTooManySystemFiles:
    return "too many files open in the system";
  case kErrorTooManyFiles:
    return "too many files open in the process";
  case kErrorFileTooBig:
    return "file too big";
  case kErrorNoSpace:
    return "no space available";
  case kErrorInvalidSeek:
    return "invalid seek offset";
  case kErrorNotWriteable:
    return "not writeable";
  case kErrorNameTooLong:
    return "name too long";
  case kErrorUnsupported:
    return "operation not supported";
  default:
    break;
  }
  return "unknown";
}
} // namespace ds2
