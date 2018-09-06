//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/Windows/Thread.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Host/Windows/ExtraWrappers.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Utils/Log.h"
#include "DebugServer2/Utils/String.h"
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

Thread::~Thread() { ::CloseHandle(_handle); }

ErrorCode Thread::terminate() {
  BOOL result = ::TerminateThread(_handle, 0);
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

  if (address.valid()) {
    CHK(modifyRegisters(
        [&address](Architecture::CPUState &state) { state.setPC(address); }));
  }

  switch (_state) {
  case kInvalid:
  case kRunning:
    DS2BUG("trying to suspend tid %" PRI_PID " in state %s", tid(),
           Stringify::ThreadState(_state));
    break;

  case kTerminated:
    return kErrorProcessNotFound;

  case kStopped:
  case kStepped: {
    if (_stopInfo.event == StopInfo::kEventStop &&
        _stopInfo.reason == StopInfo::kReasonNone) {
      DWORD result = ::ResumeThread(_handle);
      if (result == (DWORD)-1) {
        return Platform::TranslateError();
      }
    } else {
      DWORD continueStatus =
          (_stopInfo.event == StopInfo::kEventStop &&
           _stopInfo.reason == StopInfo::kReasonUserException)
              ? DBG_EXCEPTION_NOT_HANDLED
              : DBG_CONTINUE;
      BOOL result = ::ContinueDebugEvent(_process->pid(), _tid, continueStatus);
      if (!result) {
        return Platform::TranslateError();
      }
    }

    _state = kRunning;
    return kSuccess;
  }
  }

  // Silence warnings without using a `default:` case for the above switch.
  DS2_UNREACHABLE();
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
    case STATUS_BREAKPOINT:
    case STATUS_SINGLE_STEP: {
      _stopInfo.event = StopInfo::kEventStop;
      auto *hwBpm = process()->hardwareBreakpointManager();
      if (hwBpm == nullptr || !hwBpm->fillStopInfo(this, _stopInfo)) {
        _stopInfo.reason = StopInfo::kReasonBreakpoint;
      }
      break;
    }
    case STATUS_ACCESS_VIOLATION:
    case STATUS_ARRAY_BOUNDS_EXCEEDED:
    case STATUS_IN_PAGE_ERROR:
    case STATUS_STACK_OVERFLOW:
    case STATUS_STACK_BUFFER_OVERRUN:
      _stopInfo.event = StopInfo::kEventStop;
      _stopInfo.reason = StopInfo::kReasonMemoryError;
      break;

    case STATUS_DATATYPE_MISALIGNMENT:
      _stopInfo.event = StopInfo::kEventStop;
      _stopInfo.reason = StopInfo::kReasonMemoryAlignment;
      break;

    case STATUS_FLOAT_DENORMAL_OPERAND:
    case STATUS_FLOAT_DIVIDE_BY_ZERO:
    case STATUS_FLOAT_INEXACT_RESULT:
    case STATUS_FLOAT_INVALID_OPERATION:
    case STATUS_FLOAT_OVERFLOW:
    case STATUS_FLOAT_STACK_CHECK:
    case STATUS_FLOAT_UNDERFLOW:
    case STATUS_INTEGER_DIVIDE_BY_ZERO:
    case STATUS_INTEGER_OVERFLOW:
      _stopInfo.event = StopInfo::kEventStop;
      _stopInfo.reason = StopInfo::kReasonMathError;
      break;

    case STATUS_ILLEGAL_INSTRUCTION:
    case STATUS_PRIVILEGED_INSTRUCTION:
      _stopInfo.event = StopInfo::kEventStop;
      _stopInfo.reason = StopInfo::kReasonInstructionError;
      break;

    case DS2_EXCEPTION_UNCAUGHT_COM:
    case DS2_EXCEPTION_UNCAUGHT_USER:
    case DS2_EXCEPTION_UNCAUGHT_WINRT:
      _stopInfo.event = StopInfo::kEventStop;
      _stopInfo.reason = StopInfo::kReasonUserException;
      break;

    default:
      DS2LOG(Warning, "unsupported exception code: %lx",
             de.u.Exception.ExceptionRecord.ExceptionCode);

    case STATUS_INVALID_DISPOSITION:
    case STATUS_NONCONTINUABLE_EXCEPTION:
      _stopInfo.event = StopInfo::kEventStop;
      _stopInfo.reason = StopInfo::kReasonInstructionError;
      break;
    }

    break;

  case LOAD_DLL_DEBUG_EVENT: {
#define CHK_SKIP(C) CHK_ACTION(C, goto skip_name)

    std::wstring name = L"<unknown>";

    ProcessInfo pi;
    CHK_SKIP(process()->getInfo(pi));

    if (de.u.LoadDll.lpImageName == nullptr) {
      goto skip_name;
    }

    uint64_t ptr;
    // ptr needs to be set to 0 or `if (ptr != 0)` might be true even if the
    // pointer is null, on 32-bit targets.
    ptr = 0;
    CHK_SKIP(process()->readMemory(
        reinterpret_cast<uint64_t>(de.u.LoadDll.lpImageName), &ptr,
        pi.pointerSize));

    if (ptr == 0) {
      goto skip_name;
    }

    // It seems like all strings passed by the kernel here are guaranteed to be
    // unicode.
    DS2ASSERT(de.u.LoadDll.fUnicode);

    name.clear();
    wchar_t c;
    do {
      if (process()->readMemory(ptr, &c, sizeof(c)) != kSuccess) {
        break;
      }
      name.append(1, c);

      ptr += sizeof(c);
    } while (c != '\0');

  skip_name:
    DS2LOG(Debug, "new DLL loaded: %s, base=%" PRI_PTR,
           ds2::Utils::WideToNarrowString(name).c_str(),
           PRI_PTR_CAST(de.u.LoadDll.lpBaseOfDll));

    if (de.u.LoadDll.hFile != NULL) {
      ::CloseHandle(de.u.LoadDll.hFile);
    }

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

  case OUTPUT_DEBUG_STRING_EVENT: {
    auto const &dsInfo = de.u.DebugString;
    DS2LOG(Debug, "inferior output a debug string: %" PRI_PTR "[%d]",
           PRI_PTR_CAST(dsInfo.lpDebugStringData), dsInfo.nDebugStringLength);

    // The length includes terminating null character.
    DS2ASSERT(dsInfo.nDebugStringLength >= 1);
    std::string buffer((dsInfo.fUnicode ? sizeof(WCHAR) : 1) *
                           (dsInfo.nDebugStringLength - 1),
                       0);

    CHKV(process()->readMemory(
        reinterpret_cast<uint64_t>(dsInfo.lpDebugStringData),
        const_cast<char *>(buffer.c_str()), buffer.size()));

    _stopInfo.debugString =
        dsInfo.fUnicode ? ds2::Utils::WideToNarrowString(std::wstring(
                              reinterpret_cast<const wchar_t *>(buffer.c_str()),
                              dsInfo.nDebugStringLength - 1))
                        : std::move(buffer);

    _state = kStopped;
    _stopInfo.event = StopInfo::kEventStop;
    _stopInfo.reason = StopInfo::kReasonDebugOutput;
  } break;

  default:
    DS2BUG("unknown debug event code: %lu", de.dwDebugEventCode);
  }
}
} // namespace Windows
} // namespace Target
} // namespace ds2
