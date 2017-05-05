//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#pragma once

#include "DebugServer2/Types.h"

//
// Appendix F The GDB Agent Expression Mechanism
//
// https://sourceware.org/gdb/onlinedocs/gdb/Bytecode-Descriptions.html
//
namespace ds2 {
namespace GDB {

struct ByteCodeVMDelegate;

class ByteCodeInterpreter {
protected:
  std::vector<int64_t> _stack;
  ByteCodeVMDelegate *_delegate;

public:
  enum /*ErrorCode*/ {
    kSuccess,
    kErrorNoDelegate,
    kErrorStackUnderflow,
    kErrorShortByteCode,
    kErrorInvalidOpcode,
    kErrorUnimplementedOpcode,
    kErrorInvalidStackOffset,
    kErrorInvalidByteCodeAddress,
    kErrorInvalidRegister,
    kErrorInvalidTraceVariable,
    kErrorCannotRecordTrace,
    kErrorDivideByZero,
    kErrorBadAddress
  };

public:
  ByteCodeInterpreter();

public:
  inline void setDelegate(ByteCodeVMDelegate *delegate) {
    _delegate = delegate;
  }
  inline ByteCodeVMDelegate *delegate() const {
    return const_cast<ByteCodeInterpreter *>(this)->_delegate;
  }

public:
  int execute(std::string const &bc);
  bool top(int64_t &value) const;

private:
  inline bool peek(size_t index, int64_t &value) const;

private:
  inline bool pop();
  inline bool pop(int64_t &value);
  inline void push(int64_t value);

private:
  int printf(size_t nargs, std::string const &format);
};

struct ByteCodeVMDelegate {
  virtual ~ByteCodeVMDelegate() {}
  virtual bool readMemory8(Address const &address, uint8_t &result) = 0;
  virtual bool readMemory16(Address const &address, uint16_t &result) = 0;
  virtual bool readMemory32(Address const &address, uint32_t &result) = 0;
  virtual bool readMemory64(Address const &address, uint64_t &result) = 0;
  virtual bool readRegister(size_t index, uint64_t &result) = 0;
  virtual bool readTraceStateVariable(size_t index, uint64_t &result) = 0;
  virtual bool writeTraceStateVariable(size_t index, uint64_t result) = 0;
  virtual bool recordTraceValue(uint64_t value) = 0;
  virtual bool recordTraceMemory(Address const &address, size_t size,
                                 bool untilZero) = 0;
};
} // namespace GDB
} // namespace ds2
