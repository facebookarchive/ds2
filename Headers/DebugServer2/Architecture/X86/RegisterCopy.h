//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

// Yes, I just decided to write macro vomit.

#pragma once

#include "DebugServer2/Architecture/CPUState.h"

namespace ds2 {
namespace Architecture {
namespace X86 {

// Macros are shit.
#define _PASTE(A, B) A##B
#define PASTE(A, B) _PASTE(A, B)

#define DO_EMPTY()                                                             \
  do {                                                                         \
  } while (0)

#define DO_COPY_GPR_32()                                                       \
  do {                                                                         \
    DO_COPY_GP_REG(ax);                                                        \
    DO_COPY_GP_REG(cx);                                                        \
    DO_COPY_GP_REG(dx);                                                        \
    DO_COPY_GP_REG(bx);                                                        \
    DO_COPY_GP_REG(si);                                                        \
    DO_COPY_GP_REG(di);                                                        \
    DO_COPY_GP_REG(bp);                                                        \
    DO_COPY_GP_REG(sp);                                                        \
    DO_COPY_GP_REG(ip);                                                        \
    DO_COPY_FLAGS();                                                           \
  } while (0)

#define DO_COPY_GPR_64()                                                       \
  do {                                                                         \
    DO_COPY_GPR_32();                                                          \
    DO_COPY_GP_REG(8);                                                         \
    DO_COPY_GP_REG(9);                                                         \
    DO_COPY_GP_REG(10);                                                        \
    DO_COPY_GP_REG(11);                                                        \
    DO_COPY_GP_REG(12);                                                        \
    DO_COPY_GP_REG(13);                                                        \
    DO_COPY_GP_REG(14);                                                        \
    DO_COPY_GP_REG(15);                                                        \
  } while (0)

#if defined(OS_WIN32)
#define DO_COPY_SS()                                                           \
  do {                                                                         \
    DO_COPY_SS_REG(cs, Cs);                                                    \
    DO_COPY_SS_REG(ss, Ss);                                                    \
    DO_COPY_SS_REG(ds, Ds);                                                    \
    DO_COPY_SS_REG(es, Es);                                                    \
    DO_COPY_SS_REG(fs, Fs);                                                    \
    DO_COPY_SS_REG(gs, Gs);                                                    \
  } while (0)
#else
#define DO_COPY_SS()                                                           \
  do {                                                                         \
    DO_COPY_SS_REG(cs, cs);                                                    \
    DO_COPY_SS_REG(ss, ss);                                                    \
    DO_COPY_SS_REG(ds, ds);                                                    \
    DO_COPY_SS_REG(es, es);                                                    \
    DO_COPY_SS_REG(fs, fs);                                                    \
    DO_COPY_SS_REG(gs, gs);                                                    \
  } while (0)
#endif

#if defined(OS_LINUX)
#define DO_COPY_EXTRA_32()                                                     \
  do {                                                                         \
    DO_COPY_REG(state.linux_gp.PASTE(orig_, STATE_GP_REG(ax)),                 \
                user.PASTE(orig_, USER_GP_REG(ax)), 0);                        \
  } while (0)
#define DO_COPY_EXTRA_64()                                                     \
  do {                                                                         \
    DO_COPY_EXTRA_32();                                                        \
    DO_COPY_REG(state.linux_gp.fs_base, user.fs_base, 0);                      \
    DO_COPY_REG(state.linux_gp.gs_base, user.gs_base, 0);                      \
  } while (0)
#else
#define DO_COPY_EXTRA_32() DO_EMPTY()
#define DO_COPY_EXTRA_64() DO_EMPTY()
#endif

#if defined(OS_LINUX) && defined(ARCH_X86)
#define USER_GP_REG(NAME) e##NAME
#define USER_SS_REG(NAME) x##NAME
#define USER_FLAGS_REG() eflags
#elif defined(OS_LINUX) && defined(ARCH_X86_64)
#define USER_GP_REG(NAME) r##NAME
#define USER_SS_REG(NAME) NAME
#define USER_FLAGS_REG() eflags
#elif defined(OS_FREEBSD) && defined(ARCH_X86_64)
#define USER_GP_REG(NAME) r_r##NAME
#define USER_SS_REG(NAME) r_##NAME
#define USER_FLAGS_REG() r_rflags
#elif defined(OS_WIN32) && defined(ARCH_X86)
#define USER_GP_REG(NAME) E##NAME
#define USER_SS_REG(NAME) Seg##NAME
#define USER_FLAGS_REG() EFlags
#elif defined(OS_WIN32) && defined(ARCH_X86_64)
#define USER_GP_REG(NAME) R##NAME
#define USER_SS_REG(NAME) Seg##NAME
#define USER_FLAGS_REG() EFlags
#endif

#define DO_COPY_GP_REG(NAME)                                                   \
  DO_COPY_REG(state.gp.STATE_GP_REG(NAME), user.USER_GP_REG(NAME), 0)
#define DO_COPY_SS_REG(S, U)                                                   \
  DO_COPY_REG(state.gp.S, user.USER_SS_REG(U), 0xffff)
#define DO_COPY_FLAGS() DO_COPY_REG(state.gp.eflags, user.USER_FLAGS_REG(), 0)

#define STATE_GP_REG(NAME) e##NAME
#define DO_COPY_REG(S, U, M) S = (M ? U & M : U)
template <typename StateType, typename UserStructType>
static inline void user_to_state32(StateType &state,
                                   UserStructType const &user) {
  DO_COPY_GPR_32();
  DO_COPY_SS();
  DO_COPY_EXTRA_32();
}
#undef DO_COPY_REG

#define DO_COPY_REG(S, U, M) U = (M ? S & M : S)
template <typename UserStructType, typename StateType>
static inline void state32_to_user(UserStructType &user,
                                   StateType const &state) {
  DO_COPY_GPR_32();
  DO_COPY_SS();
  DO_COPY_EXTRA_32();
}
#undef DO_COPY_REG
#undef STATE_GP_REG

#define STATE_GP_REG(NAME) r##NAME
#define DO_COPY_REG(S, U, M) S = (M ? U & M : U)
template <typename StateType, typename UserStructType>
static inline void user_to_state64(StateType &state,
                                   UserStructType const &user) {
  DO_COPY_GPR_64();
  DO_COPY_SS();
  DO_COPY_EXTRA_64();
}
#undef DO_COPY_REG

#define DO_COPY_REG(S, U, M) U = (M ? S & M : S)
template <typename UserStructType, typename StateType>
static inline void state64_to_user(UserStructType &user,
                                   StateType const &state) {
  DO_COPY_GPR_64();
  DO_COPY_SS();
  DO_COPY_EXTRA_64();
}
#undef DO_COPY_REG
#undef STATE_GP_REG
} // namespace X86
} // namespace Architecture
} // namespace ds2
