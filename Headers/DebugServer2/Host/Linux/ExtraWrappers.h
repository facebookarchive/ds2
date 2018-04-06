//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#pragma once

#include "DebugServer2/Base.h"
#include "DebugServer2/Utils/CompilerSupport.h"

#include <asm/unistd.h>
#include <fcntl.h>
#include <signal.h>
#if defined(HAVE_SYS_PERSONALITY_H)
#include <sys/personality.h>
#endif
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <unistd.h>

#if !defined(DOXYGEN)

#if defined(ARCH_ARM) && !defined(ARM_VFPREGS_SIZE)
#define ARM_VFPREGS_SIZE (32 * 8 + 4)
#endif // ARCH_ARM && !ARM_VFPREGS_SIZE

#if defined(ARCH_X86) || defined(ARCH_X86_64)
// Required structs for PTrace GETREGSET with NT_X86_STATE
// These structs are not made available by the system headers
struct YMMHighVector {
  uint8_t value[16];
};

struct xstate_hdr {
  uint64_t mask;
  uint64_t reserved1[2];
  uint64_t reserved2[5];
} DS2_ATTRIBUTE_PACKED;

#if !defined(NT_X86_XSTATE)
#define NT_X86_XSTATE 0x202
#endif // !NT_X86_XSTATE

#if !defined(HAVE_STRUCT_USER_FPXREGS_STRUCT)
struct user_fpxregs_struct {
  unsigned short cwd;
  unsigned short swd;
  unsigned short twd;
  unsigned short fop;
  long fip;
  long fcs;
  long foo;
  long fos;
  long mxcsr;
  long reserved;
  long st_space[32];
  long xmm_space[32];
  long padding[56];
};
#endif // !HAVE_STRUCT_USER_FPXREGS_STRUCT

#if defined(ARCH_X86)
struct xfpregs_struct {
  user_fpxregs_struct fpregs;
  xstate_hdr header;
  YMMHighVector ymmh[8];
} DS2_ATTRIBUTE_PACKED DS2_ATTRIBUTE_ALIGNED(64);
#elif defined(ARCH_X86_64)
struct xfpregs_struct {
  user_fpregs_struct fpregs;
  xstate_hdr header;
  YMMHighVector ymmh[16];
} DS2_ATTRIBUTE_PACKED DS2_ATTRIBUTE_ALIGNED(64);
#endif
#endif // ARCH_X86 || ARCH_X86_64

#if !defined(HAVE_POSIX_OPENPT)
// Older android sysroots don't have `posix_openpt` but they all use the
// `/dev/ptmx` device.
static inline int posix_openpt(int flags) { return ::open("/dev/ptmx", flags); }
#endif // !HAVE_POSIX_OPENPT

// Do not use PLATFORM_ANDROID here. We want to test if we're using the Android
// toolchain, not if we're building for the Android target. In some cases, we
// build Tizen binaries with the Android toolchain.
#if !defined(HAVE_GETTID)
// Android headers do have a wrapper for `gettid`, unlike glibc.
static inline pid_t gettid() { return ::syscall(SYS_gettid); }
#endif // !HAVE_GETTID

#if !defined(SYS_tkill)
#define SYS_tkill __NR_tkill
#endif // !SYS_tkill

#if !defined(HAVE_TGKILL)
#if !defined(SYS_tgkill)
#define SYS_tgkill __NR_tgkill
#endif // !SYS_tgkill

static inline int tgkill(pid_t pid, pid_t tid, int signo) {
  return ::syscall(SYS_tgkill, pid, tid, signo);
}
#endif // !HAVE_TGKILL

static inline int tkill(pid_t tid, int signo) {
  return ::syscall(SYS_tkill, tid, signo);
}

#if !defined(HAVE_SYS_PERSONALITY_H)
#if !defined(SYS_personality)
#define SYS_personality __NR_personality
#endif // !SYS_personality

#if !defined(ADDR_NO_RANDOMIZE)
#define ADDR_NO_RANDOMIZE 0x0040000
#endif // !ADDR_NO_RANDOMIZE

static inline int personality(unsigned long persona) {
  return ::syscall(SYS_personality, persona);
}
#endif // !HAVE_SYS_PERSONALITY_H

#if defined(ARCH_ARM)
#if !defined(PTRACE_GETHBPREGS)
#define PTRACE_GETHBPREGS 29
#endif // !PTRACE_GETHBPREGS

#if !defined(PTRACE_SETHBPREGS)
#define PTRACE_SETHBPREGS 30
#endif // !PTRACE_SETHBPREGS
#endif // ARCH_ARM

#if !defined(PTRACE_GETREGSET)
#define PTRACE_GETREGSET 0x4204
#endif // !PTRACE_GETREGSET

#if !defined(PTRACE_SETREGSET)
#define PTRACE_SETREGSET 0x4205
#endif // !PTRACE_SETREGSET

// As defined in <asm-generic/siginfo.h>, missing in glibc
#if !defined(TRAP_BRKPT)
#define TRAP_BRKPT 1
#endif
#if !defined(TRAP_TRACE)
#define TRAP_TRACE 2
#endif
#if !defined(TRAP_BRANCH)
#define TRAP_BRANCH 3
#endif
#if !defined(TRAP_HWBKPT)
#define TRAP_HWBKPT 4
#endif

#endif // !DOXYGEN
