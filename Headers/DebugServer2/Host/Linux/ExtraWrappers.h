//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Host_Linux_ExtraWrappers_h
#define __DebugServer2_Host_Linux_ExtraWrappers_h

#include "DebugServer2/Base.h"

#include <asm/unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <unistd.h>

#if !defined(PLATFORM_ANDROID)
// Android headers do have a wrapper for `gettid`, unlike glibc.
static inline pid_t gettid() { return ::syscall(SYS_gettid); }
#endif

static inline int tgkill(pid_t pid, pid_t tid, int signo) {
  return ::syscall(SYS_tgkill, pid, tid, signo);
}

static inline int tkill(pid_t tid, int signo) {
  return ::syscall(SYS_tkill, tid, signo);
}

#endif // !__DebugServer2_Host_Linux_ExtraWrappers_h
