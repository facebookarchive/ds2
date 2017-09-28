//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#define STRINGIFY_H_INTERNAL
#include "DebugServer2/Utils/Stringify.h"
#include "DebugServer2/Utils/Log.h"

namespace ds2 {
namespace Utils {

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
    DO_DEFAULT("unknown error code", error)
  }
}

char const *Stringify::ThreadState(Target::ThreadBase::State state) {
  switch (state) {
    DO_STRINGIFY_ALIAS(Target::ThreadBase::kInvalid, kInvalid)
    DO_STRINGIFY_ALIAS(Target::ThreadBase::kRunning, kRunning)
    DO_STRINGIFY_ALIAS(Target::ThreadBase::kStepped, kStepped)
    DO_STRINGIFY_ALIAS(Target::ThreadBase::kStopped, kStopped)
    DO_STRINGIFY_ALIAS(Target::ThreadBase::kTerminated, kTerminated)
    DO_DEFAULT("unknown thread state", state)
  }
}

char const *Stringify::StopEvent(StopInfo::Event event) {
  switch (event) {
    DO_STRINGIFY(StopInfo::kEventNone)
    DO_STRINGIFY(StopInfo::kEventStop)
    DO_STRINGIFY(StopInfo::kEventExit)
    DO_STRINGIFY(StopInfo::kEventKill)
    DO_DEFAULT("unknown StopInfo event", event)
  }
}

char const *Stringify::StopReason(StopInfo::Reason reason) {
  switch (reason) {
    DO_STRINGIFY(StopInfo::kReasonNone)
    DO_STRINGIFY(StopInfo::kReasonWriteWatchpoint)
    DO_STRINGIFY(StopInfo::kReasonReadWatchpoint)
    DO_STRINGIFY(StopInfo::kReasonAccessWatchpoint)
    DO_STRINGIFY(StopInfo::kReasonBreakpoint)
    DO_STRINGIFY(StopInfo::kReasonTrace)
    DO_STRINGIFY(StopInfo::kReasonSignalStop)
    DO_STRINGIFY(StopInfo::kReasonTrap)
    DO_STRINGIFY(StopInfo::kReasonThreadSpawn)
    DO_STRINGIFY(StopInfo::kReasonThreadEntry)
    DO_STRINGIFY(StopInfo::kReasonThreadExit)
#if defined(OS_WIN32)
    DO_STRINGIFY(StopInfo::kReasonMemoryError)
    DO_STRINGIFY(StopInfo::kReasonMemoryAlignment)
    DO_STRINGIFY(StopInfo::kReasonMathError)
    DO_STRINGIFY(StopInfo::kReasonInstructionError)
    DO_STRINGIFY(StopInfo::kReasonLibraryEvent)
    DO_STRINGIFY(StopInfo::kReasonDebugOutput)
    DO_STRINGIFY(StopInfo::kReasonUserException)
#endif
    DO_DEFAULT("unknown StopInfo reason", reason)
  }
}
} // namespace Utils
} // namespace ds2
