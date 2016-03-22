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

#include "DebugServer2/Utils/CompilerSupport.h"

#include <fcntl.h>
// Older android native sysroots don't have sys/personality.h.
#if defined(HAVE_SYS_PERSONALITY_H)
#include <sys/personality.h>
#else
#include <linux/personality.h>
#undef personality
#endif
#include <unistd.h>

#if defined(ARCH_X86) && defined(PLATFORM_ANDROID)
#include <sys/user.h>
#endif

extern "C" {

pid_t gettid() DS2_ATTRIBUTE_WEAK;

int personality(unsigned long persona) DS2_ATTRIBUTE_WEAK;

// Older android native sysroots don't have posix_openpt.
int posix_openpt(int flags) DS2_ATTRIBUTE_WEAK;

// No glibc wrapper for tgkill
int tgkill(pid_t pid, pid_t tid, int signo) DS2_ATTRIBUTE_WEAK;

// No glibc wrapper for tkill
int tkill(pid_t tid, int signo) DS2_ATTRIBUTE_WEAK;
}

#endif // !__DebugServer2_Host_Linux_ExtraWrappers_h
