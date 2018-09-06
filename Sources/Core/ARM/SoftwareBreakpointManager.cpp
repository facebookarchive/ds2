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
#include "DebugServer2/Architecture/ARM/Branching.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Utils/Log.h"

#include <algorithm>
#include <cstdlib>

#define super ds2::BreakpointManager

namespace ds2 {

ErrorCode SoftwareBreakpointManager::add(Address const &address,
                                         Lifetime lifetime, size_t size,
                                         Mode mode) {
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
      CHK(_process->currentThread()->readCPUState(state));
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
      CHK(_process->readMemory(address.value() & ~1ULL, &insn, sizeof(insn)));
      auto inst_size = Architecture::ARM::GetThumbInstSize(insn);
      size = inst_size == Architecture::ARM::ThumbInstSize::TwoByteInst ? 2 : 3;
    } else {
      size = 4;
    }
  }

  return super::add(address.value() & ~1ULL, lifetime, size, mode);
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

int SoftwareBreakpointManager::hit(Target::Thread *thread, Site &site) {
  ds2::Architecture::CPUState state;
  thread->readCPUState(state);
#if defined(OS_WIN32)
  state.setPC(state.pc() - 2);
  if (super::hit(state.pc(), site)) {
    //
    // Move the PC back to the instruction
    //
    if (thread->writeCPUState(state) != kSuccess) {
      abort();
    }
    return 0;
  }
  return -1;
#else
  return super::hit(state.pc(), site) ? 0 : -1;
#endif
}

void SoftwareBreakpointManager::getOpcode(uint32_t type,
                                          ByteVector &opcode) const {
#if defined(OS_WIN32) && defined(ARCH_ARM)
  if (type == 4) {
    static const uint32_t WinARMBPType = 2;
    DS2LOG(Warning,
           "requesting a breakpoint of size %u on Windows ARM, "
           "adjusting to type %u",
           type, WinARMBPType);
    type = WinARMBPType;
  }
#endif

  opcode.clear();

  // TODO: We shouldn't have preprocessor checks for ARCH_ARM vs ARCH_ARM64
  // because we might be an ARM64 binary debugging an ARM inferior.
  switch (type) {
#if defined(ARCH_ARM)
  case 2: // udf #1
    opcode.push_back('\xde');
#if defined(OS_POSIX)
    opcode.push_back('\x01');
#elif defined(OS_WIN32)
    opcode.push_back('\xfe');
#endif
    break;
  case 3: // udf.w #0
    opcode.push_back('\xa0');
    opcode.push_back('\x00');
    opcode.push_back('\xf7');
    opcode.push_back('\xf0');
    break;
  case 4: // udf #16
    opcode.push_back('\xe7');
    opcode.push_back('\xf0');
    opcode.push_back('\x01');
    opcode.push_back('\xf0');
    break;
#elif defined(ARCH_ARM64)
  case 4:
    opcode.push_back('\xd4');
    opcode.push_back('\x20');
    opcode.push_back('\x20');
    opcode.push_back('\x00');
    break;
#endif
  default:
    DS2LOG(Error, "invalid breakpoint type %d", type);
    DS2BUG("invalid breakpoint type");
    break;
  }

#if !(defined(ENDIAN_BIG) || defined(ENDIAN_LITTLE))
#error "Target not supported."
#endif

#if defined(ENDIAN_LITTLE)
  std::reverse(opcode.begin(), opcode.end());
#endif
}

ErrorCode SoftwareBreakpointManager::isValid(Address const &address,
                                             size_t size, Mode mode) const {
  DS2ASSERT(mode == kModeExec);
  if (size < 2 || size > 4) {
    DS2LOG(Debug, "Received unsupported breakpoint size %zu", size);
    return kErrorInvalidArgument;
  }

  return super::isValid(address, size, mode);
}

size_t SoftwareBreakpointManager::chooseBreakpointSize() const {
  DS2BUG(
      "Choosing a software breakpoint size on ARM is an unsupported operation");
}
} // namespace ds2
