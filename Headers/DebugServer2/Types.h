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

#include "DebugServer2/Base.h"
#include "DebugServer2/Constants.h"
#include "DebugServer2/Core/CPUTypes.h"
#include "DebugServer2/Core/ErrorCodes.h"

#include <cstdint>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>
#if !defined(OS_WIN32)
#include <unistd.h>
#endif

namespace ds2 {

//
// Basic Types
//

#if defined(OS_WIN32)
typedef DWORD ProcessId;
typedef DWORD ThreadId;
typedef PSID UserId;
typedef PSID GroupId;
#define PRI_PID "lu"
#else
typedef pid_t ProcessId;
typedef pid_t ThreadId;
typedef uid_t UserId;
typedef gid_t GroupId;
#define PRI_PID "d"
#endif

static const ProcessId kAllProcessId = static_cast<ProcessId>(-1);
static const ProcessId kAnyProcessId = static_cast<ProcessId>(0);
static const ThreadId kAllThreadId = static_cast<ThreadId>(-1);
static const ThreadId kAnyThreadId = static_cast<ThreadId>(0);

typedef std::vector<uint8_t> ByteVector;

//
// Process/Thread Id Tuple
//
struct ProcessThreadId {
  ProcessId pid;
  ThreadId tid;

  ProcessThreadId(ProcessId _pid = kAnyProcessId, ThreadId _tid = kAnyThreadId)
      : pid(_pid), tid(_tid) {}

  inline bool validPid() const {
    return pid != kAllProcessId && pid != kAnyProcessId;
  }

  inline bool validTid() const {
    return tid != kAllThreadId && tid != kAnyThreadId;
  }

  inline bool valid() const { return validPid() || validTid(); }

  inline bool any() const { return !validPid() && !validTid(); }

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
    _unset = false;
    _value = address;
    return *this;
  }

public:
  inline bool valid() const { return !_unset; }

public:
  inline uint64_t value() const { return _value; }

public:
  inline void unset() {
    _unset = true;
    _value = 0;
  }
  inline void clear() { unset(); }
};

//
// Repesents the stop information of a process.
//

struct StopInfo {
  enum Event {
    kEventNone,
    kEventStop,
    kEventExit,
    kEventKill,
  };

  enum Reason {
    kReasonNone,
    kReasonWriteWatchpoint,
    kReasonReadWatchpoint,
    kReasonAccessWatchpoint,
    kReasonBreakpoint,
    kReasonTrace,
    kReasonSignalStop, // TODO better name
    kReasonTrap,
    kReasonThreadSpawn,
    kReasonThreadEntry,
    kReasonThreadExit,
#if defined(OS_WIN32)
    kReasonMemoryError,
    kReasonMemoryAlignment,
    kReasonMathError,
    kReasonInstructionError,
    kReasonLibraryEvent,
    kReasonDebugOutput,
    kReasonUserException,
#endif
  };

  Event event;
  Reason reason;
  // TODO: status and signal should be an union.
  int status;
  int signal;
#if defined(OS_WIN32)
  std::string debugString;
#endif

  int core;

  Address watchpointAddress;
  int watchpointIndex;

  StopInfo() { clear(); }

  inline void clear() {
    event = kEventNone;
    reason = kReasonNone;
    status = 0;
    signal = 0;
#if defined(OS_WIN32)
    debugString.clear();
#endif
    core = -1;
    watchpointAddress = 0;
    watchpointIndex = -1;
  }
};

//
// Constant for invalid native CPU (sub)type.
//
static uint32_t const kInvalidCPUType = static_cast<uint32_t>(-2);

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
  uint32_t defaultPacketTimeout;

  HostInfo() { clear(); }

  inline void clear() {
    cpuType = kCPUTypeAny;
    cpuSubType = kCPUSubTypeInvalid;
    nativeCPUType = kInvalidCPUType;
    nativeCPUSubType = kInvalidCPUType;
    endian = kEndianUnknown;
    pointerSize = 0;
    archFlags = 0;
    defaultPacketTimeout = 0;
  }
};

//
// Describe a process
//
struct ProcessInfo {
  typedef std::vector<ProcessInfo> Collection;

  ProcessId pid;
#if !defined(OS_WIN32)
  ProcessId parentPid;
#endif

  std::string name;

  UserId realUid;
  GroupId realGid;
#if !defined(OS_WIN32)
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
#if !defined(OS_WIN32)
    parentPid = kAnyProcessId;
#endif

    name.clear();

#if defined(OS_WIN32)
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

#if defined(OS_WIN32)
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
  std::string name;
#if defined(OS_LINUX)
  std::string backingFile;
  uint64_t backingFileOffset;
  uint64_t backingFileInode;
#endif

  MemoryRegionInfo() { clear(); }
  MemoryRegionInfo(Address const &start_, uint64_t length_,
                   uint64_t protection_, std::string name_) {
    clear();
    start = start_;
    length = length_;
    protection = protection_;
    name = name_;
  }

  inline void clear() {
    start.clear();
    length = 0;
    protection = 0;
    name.clear();
#if defined(OS_LINUX)
    backingFile.clear();
    backingFileOffset = 0;
    backingFileInode = 0;
#endif
  }
};

struct SharedLibraryInfo {
  std::string path;
  bool main;
  struct {
    uint64_t mapAddress;
    uint64_t baseAddress;
    uint64_t ldAddress;
  } svr4;
  std::vector<uint64_t> sections;
};

struct MappedFileInfo {
  std::string path;
  uint64_t baseAddress;
  uint64_t size;
};
} // namespace ds2
