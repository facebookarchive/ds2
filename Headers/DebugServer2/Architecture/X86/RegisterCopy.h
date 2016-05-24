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

#ifndef __DebugServer2_Architecture_X86_RegisterCopy_h
#define __DebugServer2_Architecture_X86_RegisterCopy_h

#include "DebugServer2/Architecture/CPUState.h"
#if defined(OS_LINUX)
#include "DebugServer2/Host/Linux/ExtraWrappers.h"
#endif

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

#if defined(OS_LINUX)
template <typename StateType>
static inline void state_to_user(struct xfpregs_struct &xfpregs,
                                 StateType const &state) {
  size_t numX87 = array_sizeof(state.x87.regs);

  // This hack is necessary because we don't handle EAVX registers
  size_t numSSEState = array_sizeof(state.sse.regs);
  size_t numSSEUser =
      sizeof(xfpregs.fpregs.xmm_space) / sizeof(state.sse.regs[0]);
  size_t numSSE = std::min(numSSEState, numSSEUser);

  // X87 State
  xfpregs.fpregs.swd = state.x87.fstw;
  xfpregs.fpregs.cwd = state.x87.fctw;
  xfpregs.fpregs.fop = state.x87.fop;

  auto st_space = reinterpret_cast<uint8_t *>(xfpregs.fpregs.st_space);
  static const size_t x87Size = sizeof(state.x87.regs[0].bytes);
  for (size_t n = 0; n < numX87; n++) {
    memcpy(st_space + n * x87Size, state.x87.regs[n].bytes, x87Size);
  }

  // SSE state
  xfpregs.fpregs.mxcsr = state.sse.mxcsr;
#if defined(ARCH_X86)
  xfpregs.fpregs.reserved = state.sse.mxcsrmask;
#elif defined(ARCH_X86_64)
  xfpregs.fpregs.mxcr_mask = state.sse.mxcsrmask;
#endif
  auto xmm_space = reinterpret_cast<uint8_t *>(xfpregs.fpregs.xmm_space);
  static const size_t sseSize = sizeof(state.sse.regs[0]);
  for (size_t n = 0; n < numSSE; n++) {
    memcpy(xmm_space + n * sseSize, &state.sse.regs[n], sseSize);
  }

  //  EAVX State
  auto ymmh = reinterpret_cast<uint8_t *>(xfpregs.ymmh);
  static const size_t avxSize = sizeof(state.avx.regs[0]);
  static const size_t ymmhSize = avxSize - sseSize;
  for (size_t n = 0; n < numSSE; n++) {
    auto avxHigh =
        reinterpret_cast<const uint8_t *>(&state.avx.regs[n]) + sseSize;
    memcpy(ymmh + n * ymmhSize, avxHigh, ymmhSize);
  }
}

template <typename StateType>
static inline void state32_to_user(struct xfpregs_struct &xfpregs,
                                   StateType const &state) {
#if defined(ARCH_X86)
  xfpregs.fpregs.twd = state.x87.ftag;
  xfpregs.fpregs.fcs = state.x87.fiseg;
  xfpregs.fpregs.fip = state.x87.fioff;
  xfpregs.fpregs.fos = state.x87.foseg;
  xfpregs.fpregs.foo = state.x87.fooff;
#elif defined(ARCH_X86_64)
  xfpregs.fpregs.ftw = state.x87.ftag;
  xfpregs.fpregs.rip =
      (static_cast<uint64_t>(state.x87.fiseg) << 32) | state.x87.fioff;
  xfpregs.fpregs.rdp =
      (static_cast<uint64_t>(state.x87.foseg) << 32) | state.x87.fooff;
#else
#error "Architecture not supported."
#endif

  state_to_user(xfpregs, state);
}

#if defined(ARCH_X86_64)
template <typename StateType>
static inline void state64_to_user(struct xfpregs_struct &xfpregs,
                                   StateType const &state) {
  xfpregs.fpregs.ftw = state.x87.ftag;
  xfpregs.fpregs.rip = state.x87.firip;
  xfpregs.fpregs.rdp = state.x87.forip;

  state_to_user(xfpregs, state);
}
#endif // ARCH_X86_64

template <typename StateType>
static inline void user_to_state(StateType &state,
                                 struct xfpregs_struct const &xfpregs) {
  size_t numX87 = array_sizeof(state.x87.regs);

  // This hack is necessary because we don't handle EAVX registers
  size_t numSSEState = array_sizeof(state.sse.regs);
  size_t numSSEUser =
      sizeof(xfpregs.fpregs.xmm_space) / sizeof(state.sse.regs[0]);
  size_t numSSE = std::min(numSSEState, numSSEUser);

  // X87 State
  state.x87.fstw = xfpregs.fpregs.swd;
  state.x87.fctw = xfpregs.fpregs.cwd;
  state.x87.fop = xfpregs.fpregs.fop;

  auto st_space = reinterpret_cast<uint8_t const *>(xfpregs.fpregs.st_space);
  static const size_t x87Size = sizeof(state.x87.regs[0].bytes);
  for (size_t n = 0; n < numX87; n++) {
    memcpy(state.x87.regs[n].bytes, st_space + n * x87Size, x87Size);
  }

  // SSE state
  state.sse.mxcsr = xfpregs.fpregs.mxcsr;
#if defined(ARCH_X86)
  state.sse.mxcsrmask = xfpregs.fpregs.reserved;
#elif defined(ARCH_X86_64)
  state.sse.mxcsrmask = xfpregs.fpregs.mxcr_mask;
#endif
  auto xmm_space = reinterpret_cast<uint8_t const *>(xfpregs.fpregs.xmm_space);
  static const size_t sseSize = sizeof(state.sse.regs[0]);
  for (size_t n = 0; n < numSSE; n++) {
    memcpy(&state.sse.regs[n], xmm_space + n * sseSize, sseSize);
  }

  //  EAVX State
  auto ymmh = reinterpret_cast<uint8_t const *>(xfpregs.ymmh);
  static const size_t avxSize = sizeof(state.avx.regs[0]);
  static const size_t ymmhSize = avxSize - sseSize;
  for (size_t n = 0; n < numSSE; n++) {
    auto avxHigh = reinterpret_cast<uint8_t *>(&state.avx.regs[n]) + sseSize;
    memcpy(avxHigh, ymmh + n * ymmhSize, ymmhSize);
  }
}

template <typename StateType>
static inline void user_to_state32(StateType &state,
                                   struct xfpregs_struct const &xfpregs) {
#if defined(ARCH_X86)
  state.x87.ftag = xfpregs.fpregs.twd;
  state.x87.fiseg = xfpregs.fpregs.fcs;
  state.x87.fioff = xfpregs.fpregs.fip;
  state.x87.foseg = xfpregs.fpregs.fos;
  state.x87.fooff = xfpregs.fpregs.foo;
#elif defined(ARCH_X86_64)
  state.x87.ftag = xfpregs.fpregs.ftw;
  state.x87.fiseg = xfpregs.fpregs.rip >> 32;
  state.x87.fioff = xfpregs.fpregs.rip;
  state.x87.foseg = xfpregs.fpregs.rdp >> 32;
  state.x87.fooff = xfpregs.fpregs.rdp;
#else
#error "Architecture not supported."
#endif

  user_to_state(state, xfpregs);
}

#if defined(ARCH_X86_64)
template <typename StateType>
static inline void user_to_state64(StateType &state,
                                   struct xfpregs_struct const &xfpregs) {
  state.x87.ftag = xfpregs.fpregs.ftw;
  state.x87.firip = xfpregs.fpregs.rip;
  state.x87.forip = xfpregs.fpregs.rdp;

  user_to_state(state, xfpregs);
}
#endif // ARCH_X86_64
#endif // OS_LINUX
}
}
}

#endif // !__DebugServer2_Architecture_X86_RegisterCopy_h
