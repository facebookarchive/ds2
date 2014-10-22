//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

struct DummyDelegate : ds2::GDB::ByteCodeInterpreterDelegate {
  virtual bool readMemory8(ds2::Address const &address, uint8_t &result) {
    return false;
  }
  virtual bool readMemory16(ds2::Address const &address, uint16_t &result) {
    return false;
  }
  virtual bool readMemory32(ds2::Address const &address, uint32_t &result) {
    if (address == 0xaabbccdd) {
      result = 0x8fffffff;
      return true;
    }
    return false;
  }
  virtual bool readMemory64(ds2::Address const &address, uint64_t &result) {
    return false;
  }
  virtual bool readRegister(size_t index, uint64_t &result) {
    switch (index) {
    case 1:
      result = 0x1234;
      return true;
    case 2:
      result = 0xabcd;
      return true;
    default:
      break;
    }
    return false;
  }
  virtual bool readTraceStateVariable(size_t index, uint64_t &result) {
    return false;
  }
  virtual bool writeTraceStateVariable(size_t index, uint64_t result) {
    return false;
  }
  virtual bool recordTraceValue(uint64_t value) { return false; }
  virtual bool recordTraceMemory(ds2::Address const &address, size_t size,
                                 bool untilZero) {
    return false;
  }
};

int main() {
  char bytecode[] = {
      kOpcodeREG,     0,              1,           kOpcodeREG, 0,
      2,              kOpcodeCONST32, 0xaa,        0xbb,       0xcc,
      0xdd,           kOpcodeREFI32,  kOpcodeSEXT, 32,         kOpcodeMUL,
      kOpcodeADD,     kOpcodeCONST32, 0x00,        0x00,       0x00,
      0x00,                                                          // fn
      kOpcodeCONST32, 0x00,           0x00,        0x00,       0x00, // chan
      kOpcodePICK,    2,                                             // arg0
      kOpcodePRINTF,  1,              0,           9,          'v',
      'a',            'l',            'u',         'e',        ':',
      '%',            'x',            '\0',        kOpcodeEND};

  ByteCodeInterpreter vm;
  DummyDelegate dd;
  vm.setDelegate(&dd);
  int err = vm.execute(std::string(&bytecode[0], sizeof(bytecode)));
  printf("err=%d\n", err);
  if (err == ByteCodeInterpreter::kSuccess) {
    int64_t value;
    if (vm.top(value)) {
      printf("Result=%#llx\n", (long long)value);
    }
  }
}
