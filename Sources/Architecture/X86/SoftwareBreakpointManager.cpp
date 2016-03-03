//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#define __DS2_LOG_CLASS_NAME__ "SoftwareBreakpointManager"

#include "DebugServer2/Architecture/X86/SoftwareBreakpointManager.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Utils/Log.h"

#include <cstdlib>

#define super ds2::BreakpointManager

namespace ds2 {
namespace Architecture {
namespace X86 {

SoftwareBreakpointManager::SoftwareBreakpointManager(Target::Process *process)
    : super(process) {}

SoftwareBreakpointManager::~SoftwareBreakpointManager() { clear(); }

void SoftwareBreakpointManager::clear() {
  super::clear();
  _insns.clear();
}

ErrorCode SoftwareBreakpointManager::add(Address const &address, Type type,
                                         size_t size) {
  DS2ASSERT(size == 0 || size == 1);
  return super::add(address, type, size);
}

bool SoftwareBreakpointManager::hit(Target::Thread *thread) {
  ds2::Architecture::CPUState state;

  //
  // Ignore hardware single-stepping.
  //
  if (thread->state() == Target::Thread::kStepped)
    return true;

  thread->readCPUState(state);
  state.setPC(state.pc() - 1);

  if (super::hit(state.pc())) {
    //
    // Move the PC back to the instruction, INT3 will move
    // the instruction pointer to the next byte.
    //
    if (thread->writeCPUState(state) != kSuccess)
      abort();

    uint64_t ex = state.pc();
    thread->readCPUState(state);
    DS2ASSERT(ex == state.pc());

    return true;
  }
  return false;
}

void SoftwareBreakpointManager::enableLocation(Site const &site) {
  uint8_t const opcode = 0xcc; // int 3
  uint8_t old;
  ErrorCode error;

  error = _process->readMemory(site.address, &old, sizeof(old));
  if (error != kSuccess) {
    DS2LOG(Error, "cannot enable breakpoint at %#lx",
           (unsigned long)site.address.value());
    return;
  }

  error = _process->writeMemory(site.address, &opcode, sizeof(opcode));
  if (error != kSuccess) {
    DS2LOG(Error, "cannot enable breakpoint at %#lx",
           (unsigned long)site.address.value());
    return;
  }

  DS2LOG(Debug, "set breakpoint instruction %#x at %#lx (saved insn %#x)",
         opcode, (unsigned long)site.address.value(), old);

  _insns[site.address] = old;
}

void SoftwareBreakpointManager::disableLocation(Site const &site) {
  ErrorCode error;
  uint8_t old = _insns[site.address];

  error = _process->writeMemory(site.address, &old, sizeof(old));
  if (error != kSuccess) {
    DS2LOG(Error, "cannot restore instruction at %#lx",
           (unsigned long)site.address.value());
    return;
  }

  DS2LOG(Debug, "reset instruction %#x at %#lx", old,
         (unsigned long)site.address.value());

  _insns.erase(site.address);
}
}
}
}
