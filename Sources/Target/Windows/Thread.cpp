//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#define __DS2_LOG_CLASS_NAME__ "Target::Thread"

#include "DebugServer2/Target/Windows/Thread.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Host/Windows/ExtraWrappers.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Utils/Log.h"
#include "DebugServer2/Utils/Stringify.h"

#include <cinttypes>

using ds2::Host::Platform;
using ds2::Utils::Stringify;

#define super ds2::Target::ThreadBase

namespace ds2 {
namespace Target {
namespace Windows {

Thread::Thread(Process *process, ThreadId tid, HANDLE handle)
    : super(process, tid), _handle(handle) {}

Thread::~Thread() { CloseHandle(_handle); }

ErrorCode Thread::terminate() {
  BOOL result = TerminateThread(_handle, 0);
  if (!result) {
    return Platform::TranslateError();
  }

  _state = kTerminated;
  return kSuccess;
}

ErrorCode Thread::suspend() {
  DWORD result = SuspendThread(_handle);
  if (result == (DWORD)-1) {
    return Platform::TranslateError();
  }

  _state = kStopped;
  _stopInfo.event = StopInfo::kEventStop;
  _stopInfo.reason = StopInfo::kReasonNone;
  return kSuccess;
}

ErrorCode Thread::resume(int signal, Address const &address) {
  // TODO(sas): Not sure how to translate the signal concept to Windows yet.
  // We'll probably have to get rid of these at some point.
  DS2ASSERT(signal == 0);
  // TODO(sas): Continuing a thread from a given address is not implemented yet.
  DS2ASSERT(!address.valid());

  ErrorCode error = kSuccess;

  if (_state == kStopped || _state == kStepped) {
    ProcessInfo info;

    error = process()->getInfo(info);
    if (error != kSuccess) {
      return error;
    }

    if (_stopInfo.event == StopInfo::kEventStop &&
        _stopInfo.reason == StopInfo::kReasonNone) {
      DWORD result = ResumeThread(_handle);
      if (result == (DWORD)-1) {
        return Platform::TranslateError();
      }
    } else {
      BOOL result = ContinueDebugEvent(_process->pid(), _tid, DBG_CONTINUE);
      if (!result) {
        return Platform::TranslateError();
      }
    }

    _state = kRunning;
  } else if (_state == kTerminated) {
    error = kErrorProcessNotFound;
  }

  return error;
}

void Thread::updateState() {
  // This function does nothing, because there's no way of querying the state
  // of a thread on Windows without being in the context of a debug event.
  // Instead, what we do is we wait for a debug event, and then call the other
  // overload of this function, `Thread::updateState(DEBUG_EVENT const &)`
  // which will be able to get accurate information about the process.
}

void Thread::updateState(DEBUG_EVENT const &de) {
  DS2ASSERT(de.dwThreadId == _tid);

  switch (de.dwDebugEventCode) {
  case EXCEPTION_DEBUG_EVENT:
    _state = kStopped;

    DS2LOG(
        Debug, "exception from inferior, tid=%lu, code=%s, address=%" PRI_PTR,
        tid(),
        Stringify::ExceptionCode(de.u.Exception.ExceptionRecord.ExceptionCode),
        PRI_PTR_CAST(de.u.Exception.ExceptionRecord.ExceptionAddress));

    switch (de.u.Exception.ExceptionRecord.ExceptionCode) {
    case EXCEPTION_BREAKPOINT:
    case EXCEPTION_SINGLE_STEP:
      _stopInfo.event = StopInfo::kEventStop;
      _stopInfo.reason = StopInfo::kReasonBreakpoint;
      break;

    case EXCEPTION_ACCESS_VIOLATION:
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
    case EXCEPTION_IN_PAGE_ERROR:
    case EXCEPTION_STACK_OVERFLOW:
      _stopInfo.event = StopInfo::kEventStop;
      _stopInfo.reason = StopInfo::kReasonMemoryError;
      break;

    case EXCEPTION_DATATYPE_MISALIGNMENT:
      _stopInfo.event = StopInfo::kEventStop;
      _stopInfo.reason = StopInfo::kReasonMemoryAlignment;
      break;

    case EXCEPTION_FLT_DENORMAL_OPERAND:
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_FLT_INEXACT_RESULT:
    case EXCEPTION_FLT_INVALID_OPERATION:
    case EXCEPTION_FLT_OVERFLOW:
    case EXCEPTION_FLT_STACK_CHECK:
    case EXCEPTION_FLT_UNDERFLOW:
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
    case EXCEPTION_INT_OVERFLOW:
      _stopInfo.event = StopInfo::kEventStop;
      _stopInfo.reason = StopInfo::kReasonMathError;
      break;

    case EXCEPTION_ILLEGAL_INSTRUCTION:
    case EXCEPTION_PRIV_INSTRUCTION:
      _stopInfo.event = StopInfo::kEventStop;
      _stopInfo.reason = StopInfo::kReasonInstructionError;
      break;

    case EXCEPTION_INVALID_DISPOSITION:
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
    case DS2_EXCEPTION_UNCAUGHT_COM:
    case DS2_EXCEPTION_UNCAUGHT_USER:
      _stopInfo.event = StopInfo::kEventStop;
      _stopInfo.reason = StopInfo::kReasonInstructionError;
      break;

    default:
      DS2BUG("unsupported exception code: %s",
             Stringify::ExceptionCode(
                 de.u.Exception.ExceptionRecord.ExceptionCode));
    }

    break;

  case LOAD_DLL_DEBUG_EVENT: {
    ErrorCode error;
    std::wstring name = L"<unknown>";

    ProcessInfo pi;
    error = process()->getInfo(pi);
    if (error != kSuccess)
      goto skip_name;

    if (de.u.LoadDll.lpImageName == nullptr)
      goto skip_name;

    uint64_t ptr;
    // ptr needs to be set to 0 or `if (ptr != 0)` might be true even if the
    // pointer is null, on 32-bit targets.
    ptr = 0;
    error = process()->readMemory(
        reinterpret_cast<uint64_t>(de.u.LoadDll.lpImageName), &ptr,
        pi.pointerSize);
    if (error != kSuccess)
      goto skip_name;

    if (ptr == 0)
      goto skip_name;

    // It seems like all strings passed by the kernel here are guaranteed to be
    // unicode.
    DS2ASSERT(de.u.LoadDll.fUnicode);

    name.clear();
    wchar_t c;
    do {
      error = process()->readMemory(ptr, &c, sizeof(c));
      if (error != kSuccess)
        break;
      name.append(1, c);

      ptr += sizeof(c);
    } while (c != '\0');

  skip_name:
    DS2LOG(Debug, "new DLL loaded: %s, base=%" PRI_PTR,
           Host::Platform::WideToNarrowString(name).c_str(),
           PRI_PTR_CAST(de.u.LoadDll.lpBaseOfDll));

    if (de.u.LoadDll.hFile != NULL)
      CloseHandle(de.u.LoadDll.hFile);

    _state = kStopped;
    _stopInfo.event = StopInfo::kEventStop;
    _stopInfo.reason = StopInfo::kReasonLibraryEvent;
  } break;

  case UNLOAD_DLL_DEBUG_EVENT:
    DS2LOG(Debug, "DLL unloaded, base=%" PRI_PTR,
           PRI_PTR_CAST(de.u.UnloadDll.lpBaseOfDll));
    _state = kStopped;
    _stopInfo.event = StopInfo::kEventStop;
    _stopInfo.reason = StopInfo::kReasonLibraryEvent;
    break;

  case EXIT_THREAD_DEBUG_EVENT:
    _state = kStopped;
    _stopInfo.event = StopInfo::kEventStop;
    _stopInfo.reason = StopInfo::kReasonThreadExit;
    break;

  case OUTPUT_DEBUG_STRING_EVENT:
    _state = kStopped;
    _stopInfo.event = StopInfo::kEventStop;
    _stopInfo.reason = StopInfo::kReasonDebugOutput;
    break;

  default:
    DS2BUG("unknown debug event code: %lu", de.dwDebugEventCode);
  }
}
}
}
}
