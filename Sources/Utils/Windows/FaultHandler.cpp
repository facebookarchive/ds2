//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/Windows/ExtraWrappers.h"
#include "DebugServer2/Utils/Backtrace.h"
#include "DebugServer2/Utils/Log.h"
#include "DebugServer2/Utils/Stringify.h"

namespace {

using ds2::Utils::Stringify;

class FaultHandler {
private:
  static LONG WINAPI exceptionHandler(EXCEPTION_POINTERS *exceptionInfo) {
    DS2LOG(
        Error, "received exception %s at address %" PRI_PTR ", crashing",
        Stringify::ExceptionCode(exceptionInfo->ExceptionRecord->ExceptionCode),
        PRI_PTR_CAST(exceptionInfo->ExceptionRecord->ExceptionAddress));
    ds2::Utils::PrintBacktrace();
    _exit(1);
  }

  void installCatcher() { SetUnhandledExceptionFilter(exceptionHandler); }

public:
  FaultHandler() { installCatcher(); }
};

FaultHandler instance;
} // namespace
