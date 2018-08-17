//
// Copyright (c) 2014-present, Facebook, Inc.
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
#include "DebugServer2/Utils/String.h"
#include "DebugServer2/Utils/Stringify.h"
#include "DebugServer2/Utils/SwapEndian.h"
#include "JSObjects/JSObjects.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <sstream>

#if defined(OS_POSIX)
#define FORMAT_ID(ID) ID
#elif defined(OS_WIN32)
#define FORMAT_ID(ID) 0
#else
#error "Target not supported."
#endif

using ds2::Utils::Stringify;

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
    flag = kSupported;
    value = string.substr(pos + 1);
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

void StopInfo::getWatchpointInfo(std::string &key, std::string &val,
                                 CompatibilityMode mode, bool encodeHex) const {
  std::ostringstream ss;

  if (mode == kCompatibilityModeLLDB) {
    key = "description";
    ss << watchpointAddress << " " << watchpointIndex;
  } else {
    switch (reason) {
    case StopInfo::kReasonWriteWatchpoint:
      key = "watch";
      break;
    case StopInfo::kReasonReadWatchpoint:
      key = "rwatch";
      break;
    case StopInfo::kReasonAccessWatchpoint:
      key = "awatch";
      break;
    default:
      DS2BUG("Watchpoint stop reason invalid");
    }
    ss << std::hex << watchpointAddress;
  }

  val = encodeHex ? ToHex(ss.str()) : ss.str();
}

void StopInfo::reasonToString(std::string &key, std::string &val,
                              CompatibilityMode mode) const {
  key = "reason";

  switch (reason) {
  case StopInfo::kReasonTrace:
    val = "trace";
    break;
  case StopInfo::kReasonBreakpoint:
    val = "breakpoint";
    break;
  case StopInfo::kReasonSignalStop:
    val = "signal";
    break;
  case StopInfo::kReasonTrap:
    val = "trap";
    break;
  case StopInfo::kReasonWriteWatchpoint:
  case StopInfo::kReasonReadWatchpoint:
  case StopInfo::kReasonAccessWatchpoint:
    if (mode == kCompatibilityModeLLDB) {
      val = "watchpoint";
    } else {
      key = "";
      val = "";
    }
    break;
#if defined(OS_WIN32)
  case StopInfo::kReasonLibraryEvent:
    key = "library";
    val = "1";
    break;
#endif
  default:
    key = "";
    val = "";
    break;
  }
}

std::string StopInfo::encodeInfo(CompatibilityMode mode,
                                 bool listThreads) const {
  std::ostringstream ss;

  CompatibilityMode threadMode =
      (mode == kCompatibilityModeLLDB) ? kCompatibilityModeLLDBThread : mode;

  ss << "thread:" << ptid.encode(threadMode);
  if (!threadName.empty()) {
    ss << ';' << "name:" << threadName;
  }
  if (!(core < 0)) {
    ss << ';' << "core:" << core;
  }

  std::string key, val;
  reasonToString(key, val, mode);

  if (!key.empty() && !val.empty()) {
    ss << ';' << key << ':' << val;
  }

  if (watchpointAddress) {
    getWatchpointInfo(key, val, mode, mode == kCompatibilityModeLLDB);
    ss << ';' << key << ':' << val;
  }

  if (reason == StopInfo::kReasonSignalStop) {
    ss << ';' << "signal:" << signal;
  }

  if (listThreads) {
    ss << ';' << "threads:";
    if (threads.empty()) {
      //
      // Best effort, send only this thread.
      //
      ss << ptid.encode(threadMode);
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

void StopInfo::encodeRegisters(std::map<std::string, std::string> &regs,
                               bool hexIndex) const {
  for (auto &reg : registers) {
    std::stringstream regNum;
    std::stringstream regVal;
    size_t regsize = reg.second.size << 3;

    if (hexIndex) {
      regNum << HEX(2) << (reg.first & 0xff);
    } else {
      regNum << DEC << reg.first;
    }

#if defined(ENDIAN_BIG)
    regVal << HEX(regsize >> 2) << reg.second.value;
#else
    regVal << HEX(regsize >> 2) << (Swap64(reg.second.value) >> (64 - regsize));
#endif

    regs.insert(std::make_pair(regNum.str(), regVal.str()));
  }
}

std::string StopInfo::encodeRegisters() const {
  std::ostringstream ss;
  bool first = true;
  std::map<std::string, std::string> regs;
  encodeRegisters(regs, true);

  for (auto &reg : regs) {
    if (!first) {
      ss << ';';
    }

    ss << reg.first << ':' << reg.second;

    first = false;
  }

  return ss.str();
}

std::string StopInfo::encode(CompatibilityMode mode, bool listThreads) const {
  // We shouldn't be trying to encode something that has no stop event.
  DS2ASSERT(event != kEventNone);

  std::ostringstream ss;
  if (event == kEventStop && mode == kCompatibilityModeGDBMultiprocess) {
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
  case kEventStop:
    ss << ((mode != kCompatibilityModeGDB) ? 'T' : 'S') << HEX(2);
#if !defined(OS_WIN32)
    ss << ((reason != StopInfo::kReasonNone) ? (signal & 0xff) : 0);
#else
    // Windows doesn't have a notion of signals but the GDB protocol still
    // needs some sort of emulation for these.
    switch (reason) {
    case StopInfo::kReasonNone:
    case StopInfo::kReasonLibraryEvent:
      ss << 0;
      break;
    case StopInfo::kReasonBreakpoint:
    case StopInfo::kReasonAccessWatchpoint:
    case StopInfo::kReasonReadWatchpoint:
    case StopInfo::kReasonWriteWatchpoint:
      ss << 5; // SIGTRAP
      break;
    case StopInfo::kReasonMemoryError:
      ss << 11; // SIGSEGV
      break;
    case StopInfo::kReasonMemoryAlignment:
      ss << 7; // SIGBUS
      break;
    case StopInfo::kReasonMathError:
      ss << 8; // SIGFPE
      break;
    case StopInfo::kReasonInstructionError:
      ss << 4; // SIGILL
      break;
    case StopInfo::kReasonUserException:
      ss << 30; // SIGUSR1
      break;
    default:
      DS2BUG("not implemented");
    }
#endif
    ss << DEC;
    break;

  case kEventExit:
    ss << 'W' << HEX(2) << (status & 0xff) << DEC;
    break;

  case kEventKill:
    ss << 'X' << HEX(2);
#if !defined(OS_WIN32)
    ss << (signal & 0xff);
#else
    ss << 9; // SIGKILL
#endif
    ss << DEC;
    break;

  default:
    DS2BUG("impossible StopInfo event: %s", Stringify::StopEvent(event));
  }

  //
  // When sending signals, LLDB expects that thread information
  // is present at the beginning, followed by the registers, while
  // GDB expects registers first.
  //
  if (event == kEventStop && mode != kCompatibilityModeGDB) {
    if (mode == kCompatibilityModeLLDB) {
      ss << encodeInfo(mode, listThreads) << ';' << encodeRegisters();
    } else {
      ss << encodeRegisters() << ';' << encodeInfo(mode, listThreads);
    }

    ss << ';';
  }
  return ss.str();
}

std::string
StopInfo::encodeWithAllThreads(CompatibilityMode mode,
                               const JSArray &threadsStopInfo) const {
  std::ostringstream ss;
  ss << encode(mode, true) << "jstopinfo:" << ToHex(threadsStopInfo.toString())
     << ";";
  return ss.str();
}

JSDictionary *StopInfo::encodeJson() const {
  auto threadObj = JSDictionary::New();

  threadObj->set("tid", JSInteger::New(ptid.tid));

  std::string key, val;
  reasonToString(key, val, kCompatibilityModeLLDB);
  if (!key.empty() && !val.empty()) {
    threadObj->set(key, JSString::New(val));
  }

  if (!threadName.empty())
    threadObj->set("name", JSString::New(threadName));

  if (core)
    threadObj->set("core", JSInteger::New(core));

  if (watchpointAddress) {
    std::string watchpointKey, watchpointVal;
    getWatchpointInfo(watchpointKey, watchpointVal, kCompatibilityModeLLDB,
                      false);
    threadObj->set(watchpointKey, JSString::New(watchpointVal));
  }

  if (reason == StopInfo::kReasonSignalStop) {
    threadObj->set("signal", JSInteger::New(signal));
  }

  auto regSet = JSDictionary::New();
  std::map<std::string, std::string> regs;
  encodeRegisters(regs, false);

  for (auto const &reg : regs) {
    regSet->set(reg.first, JSString::New(reg.second));
  }

  threadObj->set("registers", regSet);

  return threadObj;
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
    ss << "os_build:" << ToHex(osBuild) << ';';
  }
  if (!osKernel.empty()) {
    ss << "os_kernel:" << ToHex(osKernel) << ';';
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
    ss << "hostname:" << ToHex(hostName) << ';';
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

  if (defaultPacketTimeout > 0)
    ss << "default_packet_timeout:" << defaultPacketTimeout << ';';

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
    ss << "name:" << ToHex(name) << ';';
    ss << "triple:" << ToHex(triple) << ';';
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
      ss << "triple:" << ToHex(triple) << ';';
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

std::string RegisterInfo::encode(int xmlSet) const {
  bool xml = (xmlSet >= 0);

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

  std::vector<std::pair<std::string, std::string>> regInfo;

  regInfo.push_back(std::make_pair("name", registerName));
  if (!alternateName.empty())
    regInfo.push_back(
        std::make_pair(xml ? "altname" : "alt-name", alternateName));

  regInfo.push_back(std::make_pair("bitsize", ds2::Utils::ToString(bitSize)));
  regInfo.push_back(std::make_pair(
      "offset", ds2::Utils::ToString(byteOffset < 0 ? 0 : byteOffset)));

  if (encodingName != nullptr)
    regInfo.push_back(std::make_pair("encoding", encodingName));

  if (formatName != nullptr)
    regInfo.push_back(std::make_pair("format", formatName));

  if (!setName.empty()) {
    if (xml)
      regInfo.push_back(
          std::make_pair("group_id", ds2::Utils::ToString(xmlSet)));
    else
      regInfo.push_back(std::make_pair("set", setName));
  }

  if (xml) {
    regInfo.push_back(std::make_pair("regnum", ds2::Utils::ToString(regno)));
  }

  if (!(ehframeRegisterIndex < 0))
    regInfo.push_back(
        std::make_pair(xml ? "ehframe_regnum" : "ehframe",
                       ds2::Utils::ToString(ehframeRegisterIndex)));

  if (!(dwarfRegisterIndex < 0))
    regInfo.push_back(std::make_pair(xml ? "dwarf_regnum" : "dwarf",
                                     ds2::Utils::ToString(dwarfRegisterIndex)));

  if (!genericName.empty())
    regInfo.push_back(std::make_pair("generic", genericName));

  if (!containerRegisters.empty()) {
    std::ostringstream contStream;
    for (size_t n = 0; n < containerRegisters.size(); n++) {
      if (n != 0)
        contStream << ',';
      contStream << (xml ? std::dec : std::hex) << containerRegisters[n]
                 << std::dec;
    }
    regInfo.push_back(std::make_pair(xml ? "value_regnums" : "container-regs",
                                     contStream.str()));
  }

  if (!invalidateRegisters.empty()) {
    std::ostringstream invStream;
    for (size_t n = 0; n < invalidateRegisters.size(); n++) {
      if (n != 0)
        invStream << ',';
      invStream << (xml ? std::dec : std::hex) << invalidateRegisters[n]
                << std::dec;
    }
    regInfo.push_back(std::make_pair(
        xml ? "invalidate_regnums" : "invalidate-regs", invStream.str()));
  }

  std::ostringstream ss;
  if (xml)
    ss << "<reg ";

  for (auto const &entry : regInfo) {
    if (xml)
      ss << entry.first << "=" << '"' << entry.second << '"' << ' ';
    else
      ss << entry.first << ":" << entry.second << ";";
  }

  if (xml)
    ss << "/>";

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
  if (!name.empty()) {
    ss << "name:" << ToHex(name) << ';';
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
} // namespace GDBRemote
} // namespace ds2
