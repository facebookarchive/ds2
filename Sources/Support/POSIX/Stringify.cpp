//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Support/Stringify.h"
#include "DebugServer2/Utils/Log.h"

#include <signal.h>

namespace ds2 {
namespace Support {
namespace POSIX {

#define DO_STRINGIFY(VALUE)                                                    \
  case VALUE:                                                                  \
    return #VALUE;

#define DO_DEFAULT(MESSAGE, VALUE)                                             \
  default:                                                                     \
    if (dieOnFail)                                                             \
      DS2BUG(MESSAGE ": %#x", VALUE);                                          \
    else                                                                       \
      return nullptr;

char const *Stringify::Signal(int signal, bool dieOnFail) {
  switch (signal) {
    DO_STRINGIFY(SIGHUP)
    DO_STRINGIFY(SIGINT)
    DO_STRINGIFY(SIGQUIT)
    DO_STRINGIFY(SIGILL)
    DO_STRINGIFY(SIGTRAP)
    DO_STRINGIFY(SIGABRT)
    DO_STRINGIFY(SIGBUS)
    DO_STRINGIFY(SIGFPE)
    DO_STRINGIFY(SIGKILL)
    DO_STRINGIFY(SIGUSR1)
    DO_STRINGIFY(SIGSEGV)
    DO_STRINGIFY(SIGUSR2)
    DO_STRINGIFY(SIGPIPE)
    DO_STRINGIFY(SIGALRM)
    DO_STRINGIFY(SIGTERM)
    DO_STRINGIFY(SIGSTKFLT)
    DO_STRINGIFY(SIGCHLD)
    DO_STRINGIFY(SIGCONT)
    DO_STRINGIFY(SIGSTOP)
    DO_STRINGIFY(SIGTSTP)
    DO_STRINGIFY(SIGTTIN)
    DO_STRINGIFY(SIGTTOU)
    DO_STRINGIFY(SIGURG)
    DO_STRINGIFY(SIGXCPU)
    DO_STRINGIFY(SIGXFSZ)
    DO_STRINGIFY(SIGVTALRM)
    DO_STRINGIFY(SIGPROF)
    DO_STRINGIFY(SIGWINCH)
    DO_STRINGIFY(SIGIO)
    DO_STRINGIFY(SIGPWR)
    DO_STRINGIFY(SIGSYS)
    DO_DEFAULT("unknown signal", signal);
  }
}

char const *Stringify::SignalCode(int signal, int code, bool dieOnFail) {
  switch (signal) {
  case SIGILL:
    switch (code) {
      DO_STRINGIFY(ILL_ILLOPC)
      DO_STRINGIFY(ILL_ILLOPN)
      DO_STRINGIFY(ILL_ILLADR)
      DO_STRINGIFY(ILL_ILLTRP)
      DO_STRINGIFY(ILL_PRVOPC)
      DO_STRINGIFY(ILL_PRVREG)
      DO_STRINGIFY(ILL_COPROC)
      DO_STRINGIFY(ILL_BADSTK)
      DO_DEFAULT("unknown code", code);
    };

  case SIGBUS:
    switch (code) {
      DO_STRINGIFY(BUS_ADRALN)
      DO_STRINGIFY(BUS_ADRERR)
      DO_STRINGIFY(BUS_OBJERR)
      DO_DEFAULT("unknown code", code);
    };

  case SIGSEGV:
    switch (code) {
      DO_STRINGIFY(SEGV_MAPERR)
      DO_STRINGIFY(SEGV_ACCERR)
      DO_DEFAULT("unknown code", code);
    };

    DO_DEFAULT("unknown signal", signal);
  }
}
}
}
}
