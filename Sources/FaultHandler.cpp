//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Log.h"

#include <cassert>
#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <signal.h>
#include <unistd.h>

namespace {

class FaultHandler {
private:
  static void signalHandler(int sig, siginfo_t *si, void *uc) {
    static std::map<int, char const *> SignalNames = {
        {SIGILL, "SIGILL"}, {SIGBUS, "SIGBUS"}, {SIGSEGV, "SIGSEGV"},
    };
    static std::map<int, std::map<int, char const *>> SignalCodeNames = {
        {SIGILL,
         {
          {ILL_ILLOPC, "ILL_ILLOPC"},
          {ILL_ILLOPN, "ILL_ILLOPN"},
          {ILL_ILLADR, "ILL_ILLADR"},
          {ILL_ILLTRP, "ILL_ILLTRP"},
          {ILL_PRVOPC, "ILL_PRVOPC"},
          {ILL_PRVREG, "ILL_PRVREG"},
          {ILL_COPROC, "ILL_COPROC"},
          {ILL_BADSTK, "ILL_BADSTK"},
         }},
        {SIGBUS,
         {
          {BUS_ADRALN, "BUS_ADRALN"},
          {BUS_ADRERR, "BUS_ADRERR"},
          {BUS_OBJERR, "BUS_OBJERR"},
         }},
        {SIGSEGV,
         {
          {SEGV_MAPERR, "SEGV_MAPERR"}, {SEGV_ACCERR, "SEGV_ACCERR"},
         }},
    };
    assert(SignalNames.find(si->si_signo) != SignalNames.end());
    assert(SignalCodeNames[si->si_signo].find(si->si_code) !=
           SignalCodeNames[si->si_signo].end());

    DS2LOG(Main, Error, "received %s with code %s at address %p, crashing",
           SignalNames[si->si_signo],
           SignalCodeNames[si->si_signo][si->si_code], si->si_addr);

    _exit(si->si_signo);
  }

  void installCatcher() {
    struct sigaction sa;

    sa.sa_sigaction = FaultHandler::signalHandler;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
  }

public:
  FaultHandler() { installCatcher(); }
};

FaultHandler instance;
}
