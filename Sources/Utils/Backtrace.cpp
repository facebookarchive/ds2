//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Utils/Backtrace.h"
#include "DebugServer2/Utils/Log.h"

#if defined(OS_DARWIN) || defined(__GLIBC__)
#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>
#elif defined(OS_WIN32)
#include <windows.h>
#endif

namespace ds2 {
namespace Utils {

#if defined(OS_DARWIN) || defined(OS_WIN32) ||                                 \
    (defined(__GLIBC__) && !defined(PLATFORM_TIZEN))
static void PrintBacktraceEntrySimple(void *address) {
  DS2LOG(Error, "%" PRI_PTR, PRI_PTR_CAST(address));
}
#endif

#if defined(OS_DARWIN) || (defined(__GLIBC__) && !defined(PLATFORM_TIZEN))
void PrintBacktrace() {
  static const int kStackSize = 100;
  static void *stack[kStackSize];
  int stackEntries = ::backtrace(stack, kStackSize);

  for (int i = 0; i < stackEntries; ++i) {
    Dl_info info;
    int res;

    res = ::dladdr(stack[i], &info);
    if (res < 0) {
      PrintBacktraceEntrySimple(stack[i]);
      continue;
    }

    char *demangled = nullptr;
    const char *name;
    if (info.dli_sname != nullptr) {
      demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &res);
      if (res == 0) {
        name = demangled;
      } else {
        name = info.dli_sname;
      }
    } else {
      name = "<unknown>";
    }

    DS2LOG(Error, "%" PRI_PTR " %s+%#" PRIxPTR " (%s)", PRI_PTR_CAST(stack[i]),
           name,
           static_cast<char *>(stack[i]) - static_cast<char *>(info.dli_saddr),
           info.dli_fname);

    ::free(demangled);
  }
}
#elif defined(OS_WIN32)
void PrintBacktrace() {
  static const int kStackSize = 62;
  static PVOID stack[kStackSize];
  int stackEntries = CaptureStackBackTrace(0, kStackSize, stack, nullptr);

  for (int i = 0; i < stackEntries; ++i) {
    PrintBacktraceEntrySimple(stack[i]);
  }
}
#else
void PrintBacktrace() { DS2LOG(Warning, "unable to print backtrace"); }
#endif
} // namespace Utils
} // namespace ds2
