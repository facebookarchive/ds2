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

#include "DebugServer2/Architecture/ARM/SoftwareBreakpointManager.h"
#include "DebugServer2/Architecture/ARM/Branching.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Utils/Log.h"

#include <cstdlib>

#define super ds2::BreakpointManager

namespace ds2 {
namespace Architecture {
namespace ARM {

SoftwareBreakpointManager::SoftwareBreakpointManager(
    Target::ProcessBase *process)
    : super(process) {}

SoftwareBreakpointManager::~SoftwareBreakpointManager() { clear(); }

void SoftwareBreakpointManager::clear() {
  super::clear();
  _insns.clear();
}

ErrorCode SoftwareBreakpointManager::add(Address const &address, Type type,
                                         size_t size, Mode mode) {
  DS2ASSERT(mode == kModeExec);

  if (size < 2 || size > 4) {
    bool isThumb = false;

    //
    // Unless the address specifies the thumb bit,
    // we look at the current CPSR.
    //
    if (address.value() & 1) {
      isThumb = true;
    } else {
      ds2::Architecture::CPUState state;
      ErrorCode error = _process->currentThread()->readCPUState(state);
      if (error != kSuccess)
        return error;
      isThumb = state.isThumb();
    }

    if (isThumb) {
      //
      // The size for breakpoints is counted in bytes, but for the
      // special case of 3 where it indicates a 2x16bit word, this in
      // order to distinguish the 32bit ARM instructions. This semantic
      // is equivalent to the one used by GDB.
      //
      uint32_t insn;
      ErrorCode error =
          _process->readMemory(address.value() & ~1ULL, &insn, sizeof(insn));
      if (error != kSuccess) {
        return error;
      }
      auto inst_size = GetThumbInstSize(insn);
      size = inst_size == ThumbInstSize::TwoByteInst ? 2 : 3;
    } else {
      size = 4;
    }
  }

  return super::add(address.value() & ~1ULL, type, size, mode);
}

ErrorCode SoftwareBreakpointManager::remove(Address const &address) {
  DS2ASSERT(!(address.value() & 1));
  return super::remove(address);
}

bool SoftwareBreakpointManager::has(Address const &address) const {
  DS2ASSERT(!(address.value() & 1));
  return super::has(address);
}

void SoftwareBreakpointManager::enumerate(
    std::function<void(Site const &)> const &cb) const {
  //
  // Remove the thumb bit.
  //
  super::enumerate([&](Site const &site) {
    if (site.address & 1) {
      Site copy(site);
      copy.address = copy.address.value() & ~1;
      cb(copy);
    } else {
      cb(site);
    }
  });
}

bool SoftwareBreakpointManager::hit(Target::Thread *thread) {
  ds2::Architecture::CPUState state;
  thread->readCPUState(state);
  return super::hit(state.pc());
}

void SoftwareBreakpointManager::getOpcode(uint32_t type,
                                          std::string &opcode) const {
  switch (type) {
#if defined(ARCH_ARM)
  case 2: // udf #1
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    opcode += '\xde';
    opcode += '\x01';
#else
    opcode += '\x01';
    opcode += '\xde';
#endif
    break;
  case 3: // udf.w #0
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    opcode += '\xa0';
    opcode += '\x00';
    opcode += '\xf7';
    opcode += '\xf0';
#else
    opcode += '\xf0';
    opcode += '\xf7';
    opcode += '\x00';
    opcode += '\xa0';
#endif
    break;
  case 4: // udf #16
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    opcode += '\xe7';
    opcode += '\xf0';
    opcode += '\x01';
    opcode += '\xf0';
#else
    opcode += '\xf0';
    opcode += '\x01';
    opcode += '\xf0';
    opcode += '\xe7';
#endif
    break;
#elif defined(ARCH_ARM64)
  case 4:
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    opcode += '\xd4';
    opcode += '\x20';
    opcode += '\x20';
    opcode += '\x00';
#else
    opcode += '\x00';
    opcode += '\x20';
    opcode += '\x20';
    opcode += '\xd4';
#endif
    break;
#endif
  default:
    DS2LOG(Error, "invalid breakpoint type %d", type);
    DS2ASSERT(0 && "invalid breakpoint type");
    break;
  }
}

ErrorCode SoftwareBreakpointManager::enableLocation(Site const &site) {
  std::string opcode;
  std::string old;
  ErrorCode error;

  getOpcode(site.size, opcode);
  old.resize(opcode.size());
  error = _process->readMemory(site.address, &old[0], old.size());
  if (error != kSuccess) {
    DS2LOG(Error, "cannot enable breakpoint at %#lx, readMemory failed",
           (unsigned long)site.address.value());
    return error;
  }

  error = _process->writeMemory(site.address, &opcode[0], opcode.size());
  if (error != kSuccess) {
    DS2LOG(Error, "cannot enable breakpoint at %#lx, writeMemory failed",
           (unsigned long)site.address.value());
    return error;
  }

  DS2LOG(Debug, "set breakpoint instruction %#lx at %#lx (saved insn %#lx)",
         (unsigned long)(site.size == 2 ? *(uint16_t *)&opcode[0]
                                        : *(uint32_t *)&opcode[0]),
         (unsigned long)site.address.value(),
         (unsigned long)(site.size == 2 ? *(uint16_t *)&old[0]
                                        : *(uint32_t *)&old[0]));

  _insns[site.address] = old;

  return kSuccess;
}

ErrorCode SoftwareBreakpointManager::disableLocation(Site const &site) {
  ErrorCode error;
  std::string old = _insns[site.address];

  error = _process->writeMemory(site.address, &old[0], old.size());
  if (error != kSuccess) {
    DS2LOG(Error, "cannot restore instruction at %#lx",
           (unsigned long)site.address.value());
    return error;
  }

  DS2LOG(Debug, "reset instruction %#lx at %#lx",
         (unsigned long)(site.size == 2 ? *(uint16_t *)&old[0]
                                        : *(uint32_t *)&old[0]),
         (unsigned long)site.address.value());

  _insns.erase(site.address);

  return kSuccess;
}
}
}
}
