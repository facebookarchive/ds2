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

#include "DebugServer2/Core/SoftwareBreakpointManager.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Target/Thread.h"
#include "DebugServer2/Utils/HexValues.h"
#include "DebugServer2/Utils/Log.h"

#include <algorithm>
#include <cstdlib>

#define super ds2::BreakpointManager

namespace ds2 {

SoftwareBreakpointManager::SoftwareBreakpointManager(
    Target::ProcessBase *process)
    : super(process) {}

SoftwareBreakpointManager::~SoftwareBreakpointManager() { clear(); }

void SoftwareBreakpointManager::clear() {
  super::clear();
  _insns.clear();
}

ErrorCode SoftwareBreakpointManager::enableLocation(Site const &site,
                                                    Target::Thread *thread) {
  ByteVector opcode;
  ByteVector old;
  ErrorCode error;

  if (thread != nullptr) {
    DS2LOG(Warning, "thread-specific software breakpoints are unsupported");
  }

  getOpcode(site.size, opcode);
  old.resize(opcode.size());
  error = _process->readMemory(site.address, old.data(), old.size());
  if (error != kSuccess) {
    DS2LOG(Error, "cannot enable breakpoint at %" PRI_PTR ", readMemory failed",
           PRI_PTR_CAST(site.address.value()));
    return error;
  }

  error = _process->writeMemory(site.address, opcode.data(), opcode.size());
  if (error != kSuccess) {
    DS2LOG(Error,
           "cannot enable breakpoint at %" PRI_PTR ", writeMemory failed",
           PRI_PTR_CAST(site.address.value()));
    return error;
  }

  DS2LOG(Debug,
         "set breakpoint instruction 0x%s at %" PRI_PTR " (saved insn 0x%s)",
         ToHex(opcode).c_str(), PRI_PTR_CAST(site.address.value()),
         ToHex(old).c_str());

  _insns[site.address] = old;

  return kSuccess;
}

ErrorCode SoftwareBreakpointManager::disableLocation(Site const &site,
                                                     Target::Thread *thread) {
  ErrorCode error;
  ByteVector old = _insns[site.address];

  if (thread != nullptr) {
    DS2LOG(Warning, "thread-specific software breakpoints are unsupported");
  }

  error = _process->writeMemory(site.address, old.data(), old.size());
  if (error != kSuccess) {
    DS2LOG(Error, "cannot restore instruction at %" PRI_PTR,
           PRI_PTR_CAST(site.address.value()));
    return error;
  }

  DS2LOG(Debug, "reset instruction 0x%s at %" PRI_PTR, ToHex(old).c_str(),
         PRI_PTR_CAST(site.address.value()));

  _insns.erase(site.address);

  return kSuccess;
}

void SoftwareBreakpointManager::afterResume() {
  //
  // Remove temporary breakpoints.
  //
  auto it = _sites.begin();
  while (it != _sites.end()) {
    it->second.type =
        static_cast<Type>(it->second.type & ~kTypeTemporaryOneShot);
    if (!it->second.type) {
      // refs should always be 0 unless we have a kTypePermanent breakpoint.
      DS2ASSERT(it->second.refs == 0);
      disableLocation(it->second);
      _sites.erase(it++);
    } else {
      it++;
    }
  }
}

bool SoftwareBreakpointManager::fillStopInfo(Target::Thread *thread,
                                             StopInfo &stopInfo) {
  BreakpointManager::Site site;
  int bpIdx = hit(thread, site);
  if (bpIdx < 0) {
    return false;
  }
  stopInfo.reason = StopInfo::kReasonBreakpoint;
  return true;
}

void SoftwareBreakpointManager::insertStashedInsns(Address const &start,
                                                   size_t length,
                                                   ByteVector &data) {
  Address end = start + length;
  for (auto const &insn : _insns) {
    Address insn_start = insn.first;
    Address insn_end = insn.first + insn.second.size();

    // no overlap between region and instruction
    if (end < insn_start || start >= insn_end) {
      continue;
    }

    // overlap begins at beginning of instruction
    if (start <= insn_start) {
      uint64_t offset = insn_start - start;
      uint64_t size = std::min((uint64_t)insn.second.size(), end - insn_start);
      memcpy(&data[offset], &insn.second[0], size);
    } else {
      uint64_t offset = start - insn_start;
      uint64_t size = std::min(insn_end - start, end - start);
      memcpy(&data[0], &insn.second[offset], size);
    }
  }
}
} // namespace ds2
