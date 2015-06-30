//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/GDBRemote/ProtocolHelpers.h"
#include "DebugServer2/GDBRemote/Types.h"
#include "DebugServer2/Utils/HexValues.h"
#include "DebugServer2/Utils/Log.h"
#include "DebugServer2/Utils/SwapEndian.h"

#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <sstream>

#if defined(OS_LINUX)
#define FORMAT_ID(ID) ID
#elif defined(OS_WIN32)
#define FORMAT_ID(ID) 0
#else
#error "Target not supported."
#endif

namespace ds2 {
namespace GDBRemote {

//
// feature+ or feature- or feature? or feature=value
//
bool Feature::parse(std::string const &string) {
  size_t pos = string.find_last_of("?+-=");
  if (pos == std::string::npos)
    return false;

  name = string.substr(0, pos);
  switch (string[pos]) {
  case '?':
    flag = kQuerySupported;
    break;
  case '+':
    flag = kSupported;
    break;
  case '-':
    flag = kNotSupported;
    break;
  case '=':
    flag = kSupported, value = string.substr(pos + 1);
    break;
  }

  return true;
}

#define HEX0 std::hex << std::setfill('0')
#define HEX(N) HEX0 << std::setw(N)
#define DEC std::dec

//
// GDB and LLDB differs in encoding the thread suffix,
// there are a total of three encodings:
//    <pid>              - GDB w/o multiprocess support
//    <tid>              - LLDB default mode
//    p<pid>.<tid>       - GDB w/ multiprocess support
//    <pid>;thread:<tid> - LLDB w/ thread suffix support
//

bool ProcessThreadId::parse(std::string const &string, CompatibilityMode mode) {
#define CHECK_AND_RESET(RESULT, RESET_VAL)                                     \
  do {                                                                         \
    if (errno != 0) {                                                          \
      (RESULT) = (RESET_VAL);                                                  \
      return false;                                                            \
    }                                                                          \
  } while (0)

  pid = kAllProcessId;
  tid = kAllThreadId;

  if (string.empty())
    return false;

  errno = 0;

  if (mode == kCompatibilityModeGDB ||
      mode == kCompatibilityModeGDBMultiprocess) {
    if (string[0] == 'p') {
      char *eptr;
      pid = strtoul(string.c_str() + 1, &eptr, 16);
      CHECK_AND_RESET(pid, kAllProcessId);
      if (*eptr++ == '.') {
        tid = strtoul(eptr, nullptr, 16);
        CHECK_AND_RESET(tid, kAllThreadId);
      }
    } else {
      pid = std::strtoul(string.c_str(), nullptr, 16);
      CHECK_AND_RESET(pid, kAllProcessId);
    }
  } else if (mode == kCompatibilityModeLLDB) {
    char *eptr;
    pid = strtoul(string.c_str(), &eptr, 16);
    CHECK_AND_RESET(pid, kAllProcessId);
    if (*eptr++ == ';') {
      if (std::strncmp(eptr, "thread:", 7) == 0) {
        tid = strtoul(eptr + 7, nullptr, 16);
        CHECK_AND_RESET(tid, kAllThreadId);
      }
    } else {
      tid = pid;
      pid = kAnyProcessId;
    }
  } else if (mode == kCompatibilityModeLLDBThread) {
    if (std::strncmp(string.c_str(), "thread:", 7) == 0) {
      tid = strtoul(string.c_str() + 7, nullptr, 16);
      CHECK_AND_RESET(tid, kAllThreadId);
    }
  } else {
    return false;
  }

  return true;
#undef CHECK_AND_RESET
}

std::string ProcessThreadId::encode(CompatibilityMode mode) const {
  std::ostringstream ss;

  if (mode == kCompatibilityModeGDB) {
    ss << HEX0 << pid;
  } else if (mode == kCompatibilityModeGDBMultiprocess) {
    if (validTid()) {
      ss << 'p';
    }

    ss << HEX0 << pid;
    if (validTid()) {
      ss << '.' << HEX0 << tid;
    }
  } else if (mode == kCompatibilityModeLLDB ||
             mode == kCompatibilityModeLLDBThread) {
    if (mode == kCompatibilityModeLLDBThread) {
      if (!validTid()) {
        ss << HEX0 << pid;
      } else {
        ss << HEX0 << tid;
      }
    } else {
      ss << HEX0 << pid;
      if (validTid()) {
        ss << ';' << "thread:" << HEX0 << tid;
      }
    }
  }

  return ss.str();
}

std::string StopCode::encodeInfo(CompatibilityMode mode) const {
  std::ostringstream ss;

  if (mode == kCompatibilityModeLLDB) {
    ss << "thread:" << ptid.encode(kCompatibilityModeLLDBThread);
  } else {
    ss << "thread:" << ptid.encode(mode);
  }
  if (!threadName.empty()) {
    ss << ';' << "name:" << threadName;
  }
  if (!(core < 0)) {
    ss << ';' << "core:" << core;
  }
  if (reason != StopInfo::kReasonNone) {
#if notyet
    ss << ';';
    switch (reason) {
    case StopInfo::kReasonWatchpoint:
      ss << "watch";
      break;
    case StopInfo::kReasonRegisterWatchpoint:
      ss << "rwatch";
      break;
    case StopInfo::kReasonAddressWatchpoint:
      ss << "awatch";
      break;
    case StopInfo::kReasonLibraryLoad:
      ss << "library";
      break;
    case StopInfo::kReasonReplayLog:
      ss << "replaylog";
      break;
    }
    ss << ':' << 1;
#endif
  }

  //
  // Encode extra information needed by LLDB.
  //
  if (mode == kCompatibilityModeLLDB) {
    if (reason != StopInfo::kReasonNone) {
      ss << ';' << "reason:";
      switch (reason) {
      case StopInfo::kReasonNone:
        break;
      case StopInfo::kReasonTrace:
        ss << "trace";
        break;
      case StopInfo::kReasonBreakpoint:
        ss << "breakpoint";
        break;
      case StopInfo::kReasonWatchpoint:
        ss << "watchpoint";
        break;
      case StopInfo::kReasonSignalStop:
        ss << "signal";
        break;
      case StopInfo::kReasonTrap:
        ss << "trap";
        break;
      case StopInfo::kReasonException:
        ss << "exception";
        break;
      case StopInfo::kReasonRegisterWatchpoint:
      case StopInfo::kReasonAddressWatchpoint:
      case StopInfo::kReasonLibraryLoad:
      case StopInfo::kReasonReplayLog:
        DS2BUG("stop reason not implemented: %d", reason);
      }
    }

    ss << ';' << "threads:";
    if (threads.empty()) {
      //
      // Best effort, send only this thread.
      //
      ss << ptid.encode(kCompatibilityModeLLDBThread);
    } else {
      bool first = true;
      for (auto &tid : threads) {
        if (!first) {
          ss << ',';
        }
        ss << HEX0 << tid;
        first = false;
      }
    }
  }

  return ss.str();
}

std::string StopCode::encodeRegisters() const {
  std::ostringstream ss;
  bool first = true;

  for (auto &regval : registers) {
    if (!first) {
      ss << ';';
    }

    size_t regsize = regval.second.size << 3;

    ss << HEX(2) << (regval.first & 0xff) << ':' << HEX(regsize >> 2)
#if defined(__BIG_ENDIAN__)
       << regval.second.value
#else
       << (Swap64(regval.second.value) >> (64 - regsize))
#endif
        ;

    first = false;
  }

  return ss.str();
}

std::string StopCode::encode(CompatibilityMode mode) const {
  std::ostringstream ss;

  if (event == kSignal && mode == kCompatibilityModeGDBMultiprocess) {
    //
    // We need to have some information in order to
    // have extended stop reason.
    //
    if (!ptid.valid() && core < 0 && reason == StopInfo::kReasonNone &&
        registers.empty()) {
      //
      // We can use the simpler form.
      //
      mode = kCompatibilityModeGDB;
    }
  }

  switch (event) {
  case kSignal:
    ss << ((mode != kCompatibilityModeGDB) ? 'T' : 'S') << HEX(2)
#if !defined(OS_WIN32)
       << ((reason != StopInfo::kReasonNone) ? (signal & 0xff) : 0)
#else
       // Windows doesn't have a notion of signals but the GDB protocol still
       // needs some sort of emulation for these.
       << (reason != StopInfo::kReasonNone ? 5 : 0)
#endif
       << DEC;
    break;

#if !defined(OS_WIN32)
  case kSignalExit:
    ss << 'X' << HEX(2) << (signal & 0xff) << DEC;
    break;
#endif

  case kCleanExit:
    ss << 'W' << HEX(2) << (status & 0xff) << DEC;
  }

  //
  // When sending signals, LLDB expects that thread information
  // is present at the beginning, followed by the registers, while
  // GDB expects registers first.
  //
  if (event == kSignal && mode != kCompatibilityModeGDB) {
    if (mode == kCompatibilityModeLLDB) {
      ss << encodeInfo(mode) << ';' << encodeRegisters();
    } else {
      ss << encodeRegisters() << ';' << encodeInfo(mode);
    }

    ss << ';';
  }

  return ss.str();
}

std::string HostInfo::encode() const {
  std::ostringstream ss;

//
// For non-Apple platforms we will send arch: for qHostInfo
// encoding, this because LLDB will assume a Mach-O target
// (and thus only iOS and MacOS X) in case cputype: and
// cpusubtype: are specified; however qProcessInfo requires
// them.
//

#if defined(__APPLE__)
  bool sForceCPUType = true;
#else
  bool sForceCPUType = false;
#endif

  if (sForceCPUType) {
    ss << "cputype:" << DEC << cpuType << ';';
    if (cpuSubType != 0) {
      ss << "cpusubtype:" << DEC << cpuSubType << ';';
    }
  } else {
    ss << "arch:" << GetArchName(cpuType, cpuSubType, endian) << ';';
  }

  ss << "ostype:" << osType << ';';
  if (!osVendor.empty()) {
    ss << "vendor:" << osVendor << ';';
  }
  if (!osBuild.empty()) {
    ss << "os_build:" << StringToHex(osBuild) << ';';
  }
  if (!osKernel.empty()) {
    ss << "os_kernel:" << StringToHex(osKernel) << ';';
  }
  if (!osVersion.empty()) {
    unsigned int major, minor, revision;

    major = minor = revision = 0;

    //
    // Parse to ensure the format is maj.min.rev.
    //
    if (std::sscanf(osVersion.c_str(), "%u.%u.%u", &major, &minor, &revision) >
        0) {
      ss << "os_version:" << DEC << major << '.' << DEC << minor << '.' << DEC
         << revision << ';';
    }
  }
  if (!hostName.empty()) {
    ss << "hostname:" << StringToHex(hostName) << ';';
  }
  ss << "endian:";
  switch (endian) {
  case kEndianBig:
    ss << "big";
    break;
  case kEndianLittle:
    ss << "little";
    break;
  case kEndianPDP:
    ss << "pdp";
    break;
  default:
    ss << "unknown";
    break;
  }
  ss << ';';
  ss << "ptrsize:" << pointerSize << ';';
  ss << "watchpoint_exceptions_received:"
     << (watchpointExceptionsReceivedBefore ? "before" : "after") << ';';

  return ss.str();
}

std::string ProcessInfo::encode(CompatibilityMode mode,
                                bool alternateVersion) const {
  std::ostringstream ss;

  std::string triple;

  if (mode == kCompatibilityModeLLDB || alternateVersion) {
    triple = GetArchName(cpuType, cpuSubType);
    triple += '-';
    if (osVendor.empty()) {
      triple += "unknown";
    } else {
      triple += osVendor;
    }
    triple += '-';
    if (osType.empty()) {
      triple += "unknown";
    } else {
      triple += osType;
    }
  }

  if (alternateVersion) {
    ss << "pid:" << DEC << pid << ';';
    ss << "uid:" << DEC << FORMAT_ID(realUid) << ';';
    ss << "gid:" << DEC << FORMAT_ID(realGid) << ';';
#if !defined(OS_WIN32)
    ss << "ppid:" << DEC << parentPid << ';';
    ss << "euid:" << DEC << effectiveUid << ';';
    ss << "egid:" << DEC << effectiveGid << ';';
#endif
    ss << "name:" << StringToHex(name) << ';';
    ss << "triple:" << StringToHex(triple) << ';';
  } else {
    ss << "pid:" << HEX0 << pid << ';';
    ss << "real-uid:" << HEX0 << FORMAT_ID(realUid) << ';';
    ss << "real-gid:" << HEX0 << FORMAT_ID(realGid) << ';';
#if !defined(OS_WIN32)
    ss << "parent-pid:" << HEX0 << parentPid << ';';
    ss << "effective-uid:" << HEX0 << effectiveUid << ';';
    ss << "effective-gid:" << HEX0 << effectiveGid << ';';
#endif
    if (mode == kCompatibilityModeLLDB) {
      ss << "triple:" << StringToHex(triple) << ';';
    } else {
      // CPU{,Sub}Type contains an `enum CPUType`, and nativeCPU{,Sub}Type
      // contains the actual value that will be sent on the wire (e.g.: for ELF
      // processes it would contain values from the ELF header).
      ss << "cputype:" << HEX0 << nativeCPUType << ';';
      if (nativeCPUSubType != 0) {
        ss << "cpusubtype:" << HEX0 << nativeCPUSubType << ';';
      }
    }
    ss << "endian:";
    switch (endian) {
    case kEndianBig:
      ss << "big";
      break;
    case kEndianLittle:
      ss << "little";
      break;
    case kEndianPDP:
      ss << "pdp";
      break;
    default:
      ss << "unknown";
      break;
    }
    ss << ';';
    ss << "ptrsize:" << pointerSize << ';';
    ss << "vendor:" << osVendor << ";";
    ss << "ostype:" << osType << ";";
  }

  return ss.str();
}

std::string RegisterInfo::encode() const {
  std::ostringstream ss;

  char const *encodingName;
  switch (encoding) {
  case kEncodingNone:
    encodingName = nullptr;
    break;
  case kEncodingUInt:
    encodingName = "uint";
    break;
  case kEncodingSInt:
    encodingName = "sint";
    break;
  case kEncodingIEEE754:
    encodingName = "ieee754";
    break;
  case kEncodingVector:
    encodingName = "vector";
    break;
  default:
    return std::string();
  }

  char const *formatName;
  switch (format) {
  case kFormatNone:
    formatName = nullptr;
    break;
  case kFormatBinary:
    formatName = "binary";
    break;
  case kFormatDecimal:
    formatName = "decimal";
    break;
  case kFormatHex:
    formatName = "hex";
    break;
  case kFormatFloat:
    formatName = "float";
    break;
  case kFormatVectorUInt8:
    formatName = "vector-uint8";
    break;
  case kFormatVectorSInt8:
    formatName = "vector-sint8";
    break;
  case kFormatVectorUInt16:
    formatName = "vector-uint16";
    break;
  case kFormatVectorSInt16:
    formatName = "vector-sint16";
    break;
  case kFormatVectorUInt32:
    formatName = "vector-uint32";
    break;
  case kFormatVectorSInt32:
    formatName = "vector-sint32";
    break;
  case kFormatVectorUInt128:
    formatName = "vector-uint128";
    break;
  case kFormatVectorFloat32:
    formatName = "vector-float32";
    break;
  default:
    return std::string();
  }

  ss << "name:" << registerName << ';';
  if (!alternateName.empty()) {
    ss << "alt-name:" << alternateName << ';';
  }
  ss << "bitsize:" << bitSize << ';';
  ss << "offset:" << (byteOffset < 0 ? 0 : byteOffset) << ';';
  if (encodingName != nullptr) {
    ss << "encoding:" << encodingName << ';';
  }
  if (formatName != nullptr) {
    ss << "format:" << formatName << ';';
  }
  if (!setName.empty()) {
    ss << "set:" << setName << ';';
  }
  if (!(gccRegisterIndex < 0)) {
    ss << "gcc:" << gccRegisterIndex << ';';
  }
  if (!(dwarfRegisterIndex < 0)) {
    ss << "dwarf:" << dwarfRegisterIndex << ';';
  }
  if (!genericName.empty()) {
    ss << "generic:" << genericName << ';';
  }
  if (!containerRegisters.empty()) {
    ss << "container-regs:";
    for (size_t n = 0; n < containerRegisters.size(); n++) {
      if (n != 0) {
        ss << ',';
      }
      ss << std::hex << containerRegisters[n] << std::dec;
    }
    ss << ';';
  }
  if (!invalidateRegisters.empty()) {
    ss << "invalidate-regs:";
    for (size_t n = 0; n < invalidateRegisters.size(); n++) {
      if (n != 0) {
        ss << ',';
      }
      ss << std::hex << invalidateRegisters[n] << std::dec;
    }
    ss << ';';
  }

  return ss.str();
}

std::string MemoryRegionInfo::encode() const {
  std::ostringstream ss;

  ss << "start:" << HEX(8) << start << DEC << ';';
  ss << "size:" << HEX(8) << length << DEC << ';';
  if (protection != 0) {
    ss << "permissions:";
    if (protection & kProtectionRead)
      ss << 'r';
    if (protection & kProtectionWrite)
      ss << 'w';
    if (protection & kProtectionExecute)
      ss << 'x';
    ss << ';';
  }

  return ss.str();
}

std::string ServerVersion::encode() const {
  std::ostringstream ss;
  ss << "name:" << name << ';';
  if (!version.empty()) {
    ss << "version:" << version << ';';
  }
  if (!patchLevel.empty()) {
    ss << "patch_level:" << patchLevel << ';';
  }
  if (!releaseName.empty()) {
    ss << "release_name:" << releaseName << ';';
  }
  ss << "build_number:" << buildNumber << ';'
     << "major_version:" << majorVersion << ';'
     << "minor_version:" << majorVersion << ';';

  return ss.str();
}

std::string ProgramResult::encode() const {
  // F,exitcode,signal,escaped-binary-data
  std::ostringstream ss;
  ss << 'F' << ',' << HEX(8) << status << DEC << ',' << HEX(8) << signal << DEC
     << ',' << Escape(output);
  return ss.str();
}
}
}
