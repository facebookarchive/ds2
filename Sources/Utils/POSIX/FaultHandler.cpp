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
#include "DebugServer2/Utils/Stringify.h"

#include <csignal>
#include <unistd.h>

namespace {

using ds2::Utils::Stringify;

class FaultHandler {
private:
  static void signalHandler(int sig, siginfo_t *si, void *uc) {
    DS2LOG(Error, "received %s with code %s at address %" PRI_PTR ", crashing",
           Stringify::Signal(si->si_signo),
           Stringify::SignalCode(si->si_signo, si->si_code),
           PRI_PTR_CAST(si->si_addr));
    ds2::Utils::PrintBacktrace();
    _exit(si->si_signo);
  }

  void installCatcher() {
    static char alt[SIGSTKSZ];
    struct sigaction sa;
    stack_t ss;

    // Allocate our own signal stack so that fault handlers work even
    // when the stack pointer is busted.
    ss.ss_sp = alt;
    ss.ss_size = sizeof(alt);
    ss.ss_flags = 0;

    sa.sa_sigaction = FaultHandler::signalHandler;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;

    sigaltstack(&ss, nullptr);
    sigaction(SIGILL, &sa, nullptr);
    sigaction(SIGBUS, &sa, nullptr);
    sigaction(SIGSEGV, &sa, nullptr);
  }

public:
  FaultHandler() { installCatcher(); }
};

FaultHandler instance;
} // namespace
