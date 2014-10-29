//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Host_Linux_ExtraSyscalls_h
#define __DebugServer2_Host_Linux_ExtraSyscalls_h

#if defined(__ANDROID__)
#include <linux/personality.h>
#undef personality
#else
#include <sys/personality.h>
#endif
#include <sys/syscall.h>
#include <unistd.h>

extern "C" {

// Some older android versions do not have a stub for wait4 and personality in
// their libc.
#if defined(__ANDROID__)
static inline int personality(unsigned long persona) {
  return ::syscall(__NR_personality, persona);
}

static inline pid_t wait4(pid_t pid, int *stat_loc, int options,
                          struct rusage *rusage) {
  return ::syscall(__NR_wait4, pid, stat_loc, options, rusage);
}
#endif

static inline int tkill(pid_t tid, int signo) {
  return ::syscall(__NR_tkill, tid, signo);
}

static inline int tgkill(pid_t pid, pid_t tid, int signo) {
  return ::syscall(__NR_tgkill, pid, tid, signo);
}

// Linux sysroot usually doesn't have a wrapper for gettid but android does.
#if !defined(__ANDROID__)
static inline pid_t gettid() { return ::syscall(__NR_gettid); }
#endif

}

#endif // !__DebugServer2_Host_Linux_ExtraSyscalls_h
