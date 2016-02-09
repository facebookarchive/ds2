//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Target_CPUState_h
#define __DebugServer2_Target_CPUState_h

#include "Debugserver2/Base.h"

namespace ds2 {
namespace Target {

struct GPRSet {
  //
  // Create a copy of this register set.
  //
  virtual GPRSet *clone() const = 0;

  //
  // Copies values from another register set.
  //
  virtual bool copy(GPRSet *set) = 0;

  //
  // Returns the size of a register.
  //
  virtual size_t size(size_t index) const = 0;

  //
  // Accessors to register set as an indexed array.
  //
  virtual uint64_t &operator[](size_t index) = 0;
  virtual uint64_t operator[](size_t index) const = 0;

  //
  // Accessors to PC and SP used for code injection.
  //
  virtual uint64_t pc() const = 0;
  virtual void setPC(uint64_t pc) = 0;

  virtual uint64_t sp() const = 0;
  virtual void setSP(uint64_t sp) = 0;
};

struct FPRSet {
  //
  // Create a copy of this register set.
  //
  virtual FPRSet *clone() const = 0;

  //
  // Copies values from another register set.
  //
  virtual bool copy(FPRSet *set) = 0;

  //
  // Returns the size of a register.
  //
  virtual size_t size(size_t index) const = 0;
};

struct CPUState {
  GPRSet *gprs;
  FPRSet *fprs;
  // SPRSet *sprs;
};
}
}

#endif // !__DebugServer2_Target_CPUState_h
