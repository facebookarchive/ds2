//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Core/SoftwareBreakpointManager.h"
#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Utils/Log.h"

#include <cstdlib>

#define super ds2::BreakpointManager

namespace ds2 {

int SoftwareBreakpointManager::hit(Target::Thread *thread, Site &site) {
  ds2::Architecture::CPUState state;

  //
  // Ignore hardware single-stepping.
  //
  if (thread->state() == Target::Thread::kStepped)
    return 0;

  thread->readCPUState(state);
  state.setPC(state.pc() - 1);

  if (super::hit(state.pc(), site)) {
    //
    // Move the PC back to the instruction, INT3 will move
    // the instruction pointer to the next byte.
    //
    if (thread->writeCPUState(state) != kSuccess)
      abort();

    uint64_t ex = state.pc();
    thread->readCPUState(state);
    DS2ASSERT(ex == state.pc());

    return 0;
  }
  return -1;
}

void SoftwareBreakpointManager::getOpcode(uint32_t type,
                                          ByteVector &opcode) const {
  DS2ASSERT(type == 1);
  opcode.clear();
  opcode.push_back('\xcc'); // int 3
}

ErrorCode SoftwareBreakpointManager::isValid(Address const &address,
                                             size_t size, Mode mode) const {
  DS2ASSERT(mode == kModeExec);
  if (size != 0 && size != 1) {
    DS2LOG(Debug, "Received unsupported breakpoint size %zu", size);
    return kErrorInvalidArgument;
  }

  return super::isValid(address, size, mode);
}

size_t SoftwareBreakpointManager::chooseBreakpointSize() const {
  // On x86 and x86_64, software breakpoints will always be of size 1
  return 1;
}
} // namespace ds2
