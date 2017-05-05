//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/GDB/ByteCodeInterpreter.h"

#include <cstdio>

namespace ds2 {
namespace GDB {

namespace {

enum Opcode {
  kOpcodeINVALID,

  kOpcodeFLOAT = 0x01,
  kOpcodeADD = 0x02,
  kOpcodeSUB = 0x03,
  kOpcodeMUL = 0x04,
  kOpcodeSDIV = 0x05,
  kOpcodeUDIV = 0x06,
  kOpcodeSREM = 0x07,
  kOpcodeUREM = 0x08,
  kOpcodeLSH = 0x09,
  kOpcodeSRSH = 0x0a,
  kOpcodeURSH = 0x0b,
  kOpcodeTRACE = 0x0c,
  kOpcodeTRACEQ = 0x0d,
  kOpcodeLNOT = 0x0e,
  kOpcodeBAND = 0x0f,
  kOpcodeBOR = 0x10,
  kOpcodeBXOR = 0x11,
  kOpcodeBNOT = 0x12,
  kOpcodeEQ = 0x13,
  kOpcodeSLT = 0x14,
  kOpcodeULT = 0x15,
  kOpcodeSEXT = 0x16,
  kOpcodeREFI8 = 0x17,
  kOpcodeREFI16 = 0x18,
  kOpcodeREFI32 = 0x19,
  kOpcodeREFI64 = 0x1a,
  kOpcodeREFF32 = 0x1b,
  kOpcodeREFF64 = 0x1c,
  kOpcodeREFFLONG = 0x1d,
  kOpcodeLTOD = 0x1e,
  kOpcodeDTOL = 0x1f,
  kOpcodeIF_GOTO = 0x20,
  kOpcodeGOTO = 0x21,
  kOpcodeCONST8 = 0x22,
  kOpcodeCONST16 = 0x23,
  kOpcodeCONST32 = 0x24,
  kOpcodeCONST64 = 0x25,
  kOpcodeREG = 0x26,
  kOpcodeEND = 0x27,
  kOpcodeDUP = 0x28,
  kOpcodePOP = 0x29,
  kOpcodeZEXT = 0x2a,
  kOpcodeSWAP = 0x2b,
  kOpcodeGETV = 0x2c,
  kOpcodeSETV = 0x2d,
  kOpcodeTRACEV = 0x2e,
  kOpcodeTRACENZ = 0x2f,
  kOpcodeTRACEQ16 = 0x30,
  // 0x31 missing
  kOpcodePICK = 0x32,
  kOpcodeROT = 0x33,
  kOpcodePRINTF = 0x34,

  kOpcodeLAST
};
}

ByteCodeInterpreter::ByteCodeInterpreter() : _delegate(nullptr) {}

bool ByteCodeInterpreter::top(int64_t &value) const {
  if (_stack.empty())
    return false;
  value = _stack.back();
  return true;
}

inline bool ByteCodeInterpreter::peek(size_t index, int64_t &value) const {
  if (index >= _stack.size())
    return false;
  value = _stack[_stack.size() - index - 1];
  return true;
}

inline bool ByteCodeInterpreter::pop(int64_t &value) {
  if (!top(value))
    return false;
  _stack.pop_back();
  return true;
}

inline bool ByteCodeInterpreter::pop() {
  int64_t value;
  return pop(value);
}

inline void ByteCodeInterpreter::push(int64_t value) {
  _stack.push_back(value);
}

int ByteCodeInterpreter::execute(std::string const &bc) {
  if (_delegate == nullptr)
    return kErrorNoDelegate;

  for (size_t pc = 0; pc < bc.size(); pc++) {
    int64_t a, b, c;
    uint8_t byte;
    uint32_t offset;
    union {
      uint8_t i8;
      uint16_t i16;
      uint32_t i32;
      uint64_t i64;
      float f32;
      double f64;
    } data;

#define POP(X)                                                                 \
  if (!pop(X))                                                                 \
    return kErrorStackUnderflow;
#define TOP(X)                                                                 \
  if (!top(X))                                                                 \
    return kErrorStackUnderflow;
#define PEEK(P, X)                                                             \
  if (!peek(P, X))                                                             \
    return kErrorStackUnderflow;

    switch (bc[pc]) {
    case kOpcodeADD:
      POP(b);
      POP(a);
      push(a + b);
      break;

    case kOpcodeSUB:
      POP(b);
      POP(a);
      push(a - b);
      break;

    case kOpcodeMUL:
      POP(b);
      POP(a);
      push(a * b);
      break;

    case kOpcodeSDIV:
      POP(b);
      POP(a);
      if (b == 0)
        return kErrorDivideByZero;
      push(a / b);
      break;

    case kOpcodeUDIV:
      POP(b);
      POP(a);
      if (b == 0)
        return kErrorDivideByZero;
      push(static_cast<uint64_t>(a) / static_cast<uint64_t>(b));
      break;

    case kOpcodeSREM:
      POP(b);
      POP(a);
      if (b == 0)
        return kErrorDivideByZero;
      push(a % b);
      break;

    case kOpcodeUREM:
      POP(b);
      POP(a);
      if (b == 0)
        return kErrorDivideByZero;
      push(static_cast<uint64_t>(a) % static_cast<uint64_t>(b));
      break;

    case kOpcodeLSH:
      POP(b);
      POP(a);
      push(a << (b & 0x3f));
      break;

    case kOpcodeSRSH:
      POP(b);
      POP(a);
      push(a >> (b & 0x3f));
      break;

    case kOpcodeURSH:
      POP(b);
      POP(a);
      push(static_cast<uint64_t>(a) >> (b & 0x3f));
      break;

    case kOpcodeTRACE:
      POP(b); // size
      POP(a); // addr
      if (!_delegate->recordTraceMemory(a, b, false))
        return kErrorCannotRecordTrace;
      break;

    case kOpcodeTRACEQ:
      TOP(a); // addr
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      offset = bc[pc];
      if (!_delegate->recordTraceMemory(a, offset, false))
        return kErrorCannotRecordTrace;
      break;

    case kOpcodeLNOT:
      POP(a);
      push(!a);
      break;

    // case kOpcodeTRACE:
    // case kOpcodeTRACEQ:

    case kOpcodeBAND:
      POP(b);
      POP(a);
      push(a & b);
      break;

    case kOpcodeBOR:
      POP(b);
      POP(a);
      push(a | b);
      break;

    case kOpcodeBXOR:
      POP(b);
      POP(a);
      push(a ^ b);
      break;

    case kOpcodeBNOT:
      POP(a);
      push(~a);
      break;

    case kOpcodeEQ:
      POP(b);
      POP(a);
      push(a == b);
      break;

    case kOpcodeSLT:
      POP(b);
      POP(a);
      push(a < b);
      break;

    case kOpcodeULT:
      POP(b);
      POP(a);
      push(static_cast<uint64_t>(a) < static_cast<uint64_t>(b));
      break;

    case kOpcodeSEXT:
      POP(a);
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      byte = bc[pc] & 0x3f;
      push(a | (-(a >> (byte - 1)) << byte));
      break;

    case kOpcodeREFI8:
      POP(a);
      if (!_delegate->readMemory8(a, data.i8))
        return kErrorBadAddress;
      push(data.i8);
      break;

    case kOpcodeREFI16:
      POP(a);
      if (!_delegate->readMemory16(a, data.i16))
        return kErrorBadAddress;
      push(data.i16);
      break;

    case kOpcodeREFI32:
      POP(a);
      if (!_delegate->readMemory32(a, data.i32))
        return kErrorBadAddress;
      push(data.i32);
      break;

    case kOpcodeREFI64:
      POP(a);
      if (!_delegate->readMemory64(a, data.i64))
        return kErrorBadAddress;
      push(data.i64);
      break;

    case kOpcodeIF_GOTO:
      POP(a);
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      offset = bc[pc];
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      offset <<= 8, offset |= bc[pc];
      if (offset >= bc.size())
        return kErrorInvalidByteCodeAddress;
      if (a != 0) {
        pc = offset -
             1; // - 1 because the PC is incremented at beginning of loop
      }
      break;

    case kOpcodeGOTO:
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      offset = bc[pc];
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      offset <<= 8, offset |= bc[pc];
      if (offset >= bc.size())
        return kErrorInvalidByteCodeAddress;
      pc = offset - 1; // - 1 because the PC is incremented at beginning of loop
      break;

    case kOpcodeCONST8:
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      push(bc[pc]);
      break;

    case kOpcodeCONST16:
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      data.i16 = bc[pc];
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      data.i16 <<= 8, data.i16 |= bc[pc];
      push(data.i16);
      break;

    case kOpcodeCONST32:
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      data.i32 = bc[pc];
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      data.i32 <<= 8, data.i32 |= bc[pc];
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      data.i32 <<= 8, data.i32 |= bc[pc];
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      data.i32 <<= 8, data.i32 |= bc[pc];
      push(data.i32);
      break;

    case kOpcodeCONST64:
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      data.i64 = bc[pc];
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      data.i64 <<= 8, data.i64 |= bc[pc];
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      data.i64 <<= 8, data.i64 |= bc[pc];
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      data.i64 <<= 8, data.i64 |= bc[pc];
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      data.i64 <<= 8, data.i64 |= bc[pc];
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      data.i64 <<= 8, data.i64 |= bc[pc];
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      data.i64 <<= 8, data.i64 |= bc[pc];
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      data.i64 <<= 8, data.i64 |= bc[pc];
      push(data.i64);
      break;

    case kOpcodeREG:
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      offset = bc[pc];
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      offset <<= 8, offset |= bc[pc];
      if (!_delegate->readRegister(offset, data.i64))
        return kErrorInvalidRegister;
      push(data.i64);
      break;

    case kOpcodeEND:
      pc = bc.size();
      break;

    case kOpcodeDUP:
      TOP(a);
      push(a);
      break;

    case kOpcodePOP:
      POP(a);
      break;

    case kOpcodeZEXT:
      POP(a);
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      byte = bc[pc] & 0x3f;
      push(a & ~(~0ULL << byte));
      break;

    case kOpcodeSWAP:
      POP(b);
      POP(a);
      push(b);
      push(a);
      break;

    case kOpcodeGETV:
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      offset = bc[pc];
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      offset <<= 8, offset |= bc[pc];
      if (!_delegate->readTraceStateVariable(offset, data.i64))
        return kErrorInvalidTraceVariable;
      push(data.i64);
      break;

    case kOpcodeSETV:
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      offset = bc[pc];
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      offset <<= 8, offset |= bc[pc];
      TOP(a);
      if (!_delegate->writeTraceStateVariable(offset, a))
        return kErrorInvalidTraceVariable;
      break;

    case kOpcodeTRACEV:
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      offset = bc[pc];
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      offset <<= 8, offset |= bc[pc];
      if (!_delegate->readTraceStateVariable(offset, data.i64))
        return kErrorInvalidTraceVariable;
      a = 0; // XXX: This should probably be a POP() call.
      if (!_delegate->recordTraceValue(a))
        return kErrorCannotRecordTrace;
      push(a);
      break;

    case kOpcodeTRACENZ:
      POP(b); // size
      POP(a); // addr
      if (!_delegate->recordTraceMemory(a, b, true))
        return kErrorCannotRecordTrace;
      break;

    case kOpcodeTRACEQ16:
      TOP(a); // addr
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      offset = bc[pc];
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      offset <<= 8, offset |= bc[pc];
      if (!_delegate->recordTraceMemory(a, offset, false))
        return kErrorCannotRecordTrace;
      break;

    case kOpcodePICK:
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      byte = bc[pc];
      if (byte >= _stack.size())
        return kErrorInvalidStackOffset;
      PEEK(byte, a);
      push(a);
      break;

    case kOpcodeROT:
      if (_stack.size() < 3)
        return kErrorStackUnderflow;
      POP(c);
      POP(b);
      POP(a);
      push(c);
      push(b);
      push(a);
      break;

    case kOpcodePRINTF: {
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      uint8_t nargs = bc[pc];
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      offset = bc[pc];
      if (++pc >= bc.size())
        return kErrorShortByteCode;
      offset <<= 8, offset |= bc[pc];
      pc++;
      if (pc + offset >= bc.size())
        return kErrorShortByteCode;
      int err = printf(
          nargs, std::string(reinterpret_cast<char const *>(&bc[pc]), offset));
      if (err != kSuccess)
        return err;
      pc += offset - 1; // -1 because pc is incremented at beginning of the loop
    } break;

    default:
      if (bc[pc] == kOpcodeINVALID || bc[pc] >= kOpcodeLAST)
        return kErrorInvalidOpcode;
      else
        return kErrorUnimplementedOpcode;
    }
  }

  return kSuccess;
}

int ByteCodeInterpreter::printf(size_t nargs, std::string const &format) {
  // TODO complete me!
  std::printf("nargs=%u format='%s'\n", (unsigned)nargs, format.c_str());
  while (nargs-- != 0) {
    if (!pop())
      return kErrorStackUnderflow;
  }
  if (!pop())
    return kErrorStackUnderflow; // channel
  if (!pop())
    return kErrorStackUnderflow; // function

  return kSuccess;
}
} // namespace GDB
} // namespace ds2
