//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Support/Stringify.h"
#include "DebugServer2/Support/StringifyPrivate.h"
#include "DebugServer2/Utils/Log.h"

namespace ds2 {
namespace Support {

char const *Stringify::Error(ErrorCode error) {
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

char const *Stringify::StopEvent(StopInfo::Event event) {
  switch (event) {
    DO_STRINGIFY(StopInfo::kEventNone)
    DO_STRINGIFY(StopInfo::kEventStop)
    DO_STRINGIFY(StopInfo::kEventExit)
    DO_STRINGIFY(StopInfo::kEventKill)
    DO_DEFAULT("unknown StopInfo event", event);
  }
}

char const *Stringify::StopReason(StopInfo::Reason reason) {
  switch (reason) {
    DO_STRINGIFY(StopInfo::kReasonNone)
    DO_STRINGIFY(StopInfo::kReasonWatchpoint)
    DO_STRINGIFY(StopInfo::kReasonRegisterWatchpoint)
    DO_STRINGIFY(StopInfo::kReasonAddressWatchpoint)
    DO_STRINGIFY(StopInfo::kReasonLibraryLoad)
    DO_STRINGIFY(StopInfo::kReasonLibraryUnload)
    DO_STRINGIFY(StopInfo::kReasonBreakpoint)
    DO_STRINGIFY(StopInfo::kReasonSignalStop)
    DO_STRINGIFY(StopInfo::kReasonTrap)
    DO_DEFAULT("unknown StopInfo reason", reason);
  }
}
}
}
