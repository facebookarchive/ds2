//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Types_h
#define __DebugServer2_Types_h

#include "DebugServer2/Base.h"
#include "DebugServer2/Constants.h"
#include "DebugServer2/CPUTypes.h"

#include <map>
#include <stdlib.h>
#include <string>
#include <vector>
#if !defined(_WIN32)
#include <unistd.h>
#endif

namespace ds2 {

//
// Basic Types
//

#if defined(_WIN32)
typedef DWORD ProcessId;
typedef DWORD ThreadId;
typedef PSID UserId;
typedef PSID GroupId;
#else
typedef pid_t ProcessId;
typedef pid_t ThreadId;
typedef uid_t UserId;
typedef gid_t GroupId;
#endif

static const ProcessId kAllProcessId = static_cast<ProcessId>(-1);
static const ProcessId kAnyProcessId = static_cast<ProcessId>(0);
static const ThreadId kAllThreadId = static_cast<ThreadId>(-1);
static const ThreadId kAnyThreadId = static_cast<ThreadId>(0);

typedef std::vector<uint8_t> U8Vector;

//
// Process/Thread Id Tuple
//
struct ProcessThreadId {
  ProcessId pid;
  ThreadId tid;

  ProcessThreadId(ProcessId pid = kAnyProcessId, ThreadId tid = kAnyThreadId) {
    ProcessThreadId::pid = pid;
    ProcessThreadId::tid = tid;
  }

  inline bool valid() const { return (pid > 0 || tid > 0); }

  inline bool any() const { return (pid <= 0 && tid <= 0); }

  inline void clear() {
    pid = kAnyProcessId;
    tid = kAnyThreadId;
  }
};

//
// STL Types
//

typedef std::vector<std::string> StringCollection;
typedef std::map<std::string, std::string> EnvironmentBlock;

//
// Repesents the stop information of a process.
//

struct TrapInfo {
  enum Event {
    kEventNone,
    kEventExit,
    kEventKill,
    kEventCoreDump,
    kEventTrap,
    kEventStop,
  };

  enum Reason {
    kReasonNone,
    kReasonThreadNew,
    kReasonThreadExit,
  };

  ProcessId pid;
  ThreadId tid;

  Event event;
  Reason reason;
  uint32_t core;
  int status;
  int signal;

  struct {
    uint32_t type;
    uint64_t data[3];
  } exception;

  TrapInfo() { clear(); }

  inline void clear() {
    pid = kAnyProcessId;
    tid = kAnyThreadId;

    core = 0;

    event = kEventNone;
    reason = kReasonNone;
    status = 0;
    signal = 0;

    exception.type = 0;
    exception.data[0] = 0;
    exception.data[1] = 0;
    exception.data[2] = 0;
  }
};

//
// Represents an address
//

class Address {
private:
  uint64_t _value;
  bool _unset;

public:
  Address() : _value(0), _unset(true) {}

  Address(uint64_t address) : _value(address), _unset(false) {}

public:
  inline operator uint64_t() const { return _value; }
  inline Address &operator=(uint64_t address) {
    _unset = false, _value = address;
    return *this;
  }

public:
  inline bool valid() const { return !_unset; }

public:
  inline uint64_t value() const { return _value; }

public:
  inline void unset() { _unset = true, _value = 0; }
  inline void clear() { unset(); }
};

//
// Constant for invalid native CPU (sub)type.
//
static uint32_t const kInvalidCPUType = -2U;

//
// Architecture Flags
//
enum {
  kArchFlagWatchpointExceptionsReceivedAfter = (0 << 0),
  kArchFlagWatchpointExceptionsReceivedBefore = (1 << 0)
};

//
// Describes the host information.
//
struct HostInfo {
  CPUType cpuType;
  CPUSubType cpuSubType;
  uint32_t nativeCPUType;
  uint32_t nativeCPUSubType;
  std::string hostName;
  std::string osType;
  std::string osVendor;
  std::string osBuild;
  std::string osKernel;
  std::string osVersion;
  Endian endian;
  size_t pointerSize;
  uint32_t archFlags;

  HostInfo() { clear(); }

  inline void clear() {
    cpuType = kCPUTypeAny;
    cpuSubType = kCPUSubTypeInvalid;
    nativeCPUType = kInvalidCPUType;
    nativeCPUSubType = kInvalidCPUType;
    endian = kEndianUnknown;
    pointerSize = 0;
    archFlags = 0;
  }
};

//
// Describe a process
//
struct ProcessInfo {
  typedef std::vector<ProcessInfo> Collection;

  ProcessId pid;
#if !defined(_WIN32)
  ProcessId parentPid;
#endif

  std::string name;

  UserId realUid;
  GroupId realGid;
#if !defined(_WIN32)
  UserId effectiveUid;
  GroupId effectiveGid;
#endif

  CPUType cpuType;
  CPUSubType cpuSubType;
  uint32_t nativeCPUType;
  uint32_t nativeCPUSubType;
  Endian endian;
  size_t pointerSize;
  uint32_t archFlags;

  std::string osType;
  std::string osVendor;

  ProcessInfo() { clear(); }

  inline void clear() {
    pid = kAnyProcessId;
#if !defined(_WIN32)
    parentPid = kAnyProcessId;
#endif

    name.clear();

#if defined(_WIN32)
    realUid = nullptr;
    realGid = nullptr;
#else
    realUid = 0;
    realGid = 0;
    effectiveUid = 0;
    effectiveGid = 0;
#endif

    cpuType = kCPUTypeAny;
    cpuSubType = kCPUSubTypeInvalid;
    nativeCPUType = kInvalidCPUType;
    nativeCPUSubType = kInvalidCPUType;
    endian = kEndianUnknown;
    pointerSize = 0;
    archFlags = 0;

    osType.clear();
    osVendor.clear();
  }

#if defined(_WIN32)
  ~ProcessInfo() {
    free(realUid);
    free(realGid);
  }
#endif
};

//
// Describes a memory region
//
struct MemoryRegionInfo {
  typedef std::vector<MemoryRegionInfo> Collection;

  Address start;
  uint64_t length;
  uint32_t protection;

  MemoryRegionInfo() { clear(); }

  inline void clear() {
    start.clear();
    length = 0;
    protection = 0;
  }
};
}

#endif // !__DebugServer2_Types_h
