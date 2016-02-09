//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/Linux/ExtraWrappers.h"

#include <cstdio>
#include <sys/syscall.h>

extern "C" {

int posix_openpt(int flags) { return ::open("/dev/ptmx", flags); }

int personality(unsigned long persona) {
  return ::syscall(__NR_personality, persona);
}

pid_t gettid() { return ::syscall(__NR_gettid); }

int tgkill(pid_t pid, pid_t tid, int signo) {
  return ::syscall(__NR_tgkill, pid, tid, signo);
}

int tkill(pid_t tid, int signo) { return ::syscall(__NR_tkill, tid, signo); }
}
