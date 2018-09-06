//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/GDBRemote/Session.h"
#include "DebugServer2/GDBRemote/ProtocolHelpers.h"
#include "DebugServer2/GDBRemote/SessionDelegate.h"
#include "DebugServer2/Utils/HexValues.h"
#include "DebugServer2/Utils/Log.h"
#include "DebugServer2/Utils/String.h"
#include "DebugServer2/Utils/SwapEndian.h"

#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <sstream>

#if defined(OS_POSIX)
#define UNPACK_ID(STR) std::strtoul(STR, nullptr, 10)
#elif defined(OS_WIN32)
#define UNPACK_ID(STR) 0
#else
#error "Target not supported."
#endif

#define CHK_SEND(C) CHK_ACTION(C, sendError(CHK_error); return )

namespace ds2 {
namespace GDBRemote {

Session::Session(CompatibilityMode mode)
    : SessionBase(mode), _threadsInStopReply(false) {
#define REGISTER_HANDLER(MODE, MESSAGE, HANDLER)                               \
  do {                                                                         \
    bool REGISTER_HANDLER_result = interpreter().registerHandler(              \
        MODE, MESSAGE, this, &Session::Handle_##HANDLER);                      \
    DS2ASSERT(REGISTER_HANDLER_result);                                        \
  } while (0)

#define REGISTER_HANDLER_EQUALS_2(MESSAGE, HANDLER)                            \
  REGISTER_HANDLER(ProtocolInterpreter::Handler::kModeEquals, MESSAGE, HANDLER)

#define REGISTER_HANDLER_EQUALS_1(HANDLER)                                     \
  REGISTER_HANDLER_EQUALS_2(#HANDLER, HANDLER)

#define REGISTER_HANDLER_STARTS_WITH_2(MESSAGE, HANDLER)                       \
  REGISTER_HANDLER(ProtocolInterpreter::Handler::kModeStartsWith, MESSAGE,     \
                   HANDLER)

#define REGISTER_HANDLER_STARTS_WITH_1(HANDLER)                                \
  REGISTER_HANDLER_STARTS_WITH_2(#HANDLER, HANDLER)

  REGISTER_HANDLER_EQUALS_2("\x03", ControlC);
  REGISTER_HANDLER_EQUALS_2("?", QuestionMark);
  REGISTER_HANDLER_EQUALS_2("!", ExclamationMark);

  REGISTER_HANDLER_EQUALS_1(A);
  REGISTER_HANDLER_EQUALS_1(B);
  REGISTER_HANDLER_EQUALS_1(b);
  REGISTER_HANDLER_EQUALS_1(bc);
  REGISTER_HANDLER_EQUALS_1(bs);
  REGISTER_HANDLER_EQUALS_1(C);
  REGISTER_HANDLER_EQUALS_1(c);
  REGISTER_HANDLER_EQUALS_1(D);
  REGISTER_HANDLER_EQUALS_1(d);
  REGISTER_HANDLER_EQUALS_1(G);
  REGISTER_HANDLER_EQUALS_1(g);
  REGISTER_HANDLER_EQUALS_1(H);
  REGISTER_HANDLER_EQUALS_1(I);
  REGISTER_HANDLER_EQUALS_1(i);
  REGISTER_HANDLER_EQUALS_1(jThreadsInfo);
  REGISTER_HANDLER_EQUALS_1(k);
  REGISTER_HANDLER_EQUALS_1(_M);
  REGISTER_HANDLER_EQUALS_1(_m);
  REGISTER_HANDLER_EQUALS_1(M);
  REGISTER_HANDLER_EQUALS_1(m);
  REGISTER_HANDLER_EQUALS_1(P);
  REGISTER_HANDLER_EQUALS_1(p);
  REGISTER_HANDLER_EQUALS_1(QAgent);
  REGISTER_HANDLER_EQUALS_1(QAllow);
  REGISTER_HANDLER_EQUALS_1(QDisableRandomization);
  REGISTER_HANDLER_EQUALS_1(QEnvironment);
  REGISTER_HANDLER_EQUALS_1(QEnvironmentHexEncoded);
  REGISTER_HANDLER_EQUALS_1(QLaunchArch);
  REGISTER_HANDLER_EQUALS_1(QListThreadsInStopReply);
  REGISTER_HANDLER_EQUALS_1(QNonStop);
  REGISTER_HANDLER_EQUALS_1(QPassSignals);
  REGISTER_HANDLER_EQUALS_1(QProgramSignals);
  REGISTER_HANDLER_EQUALS_1(QRestoreRegisterState);
  REGISTER_HANDLER_EQUALS_1(QSaveRegisterState);
  REGISTER_HANDLER_EQUALS_1(QSetDisableASLR);
  REGISTER_HANDLER_EQUALS_1(QSetEnableAsyncProfiling);
  REGISTER_HANDLER_EQUALS_1(QSetLogging);
  REGISTER_HANDLER_EQUALS_1(QSetMaxPacketSize);
  REGISTER_HANDLER_EQUALS_1(QSetMaxPayloadSize);
  REGISTER_HANDLER_EQUALS_1(QSetSTDERR);
  REGISTER_HANDLER_EQUALS_1(QSetSTDIN);
  REGISTER_HANDLER_EQUALS_1(QSetSTDOUT);
  REGISTER_HANDLER_EQUALS_1(QSetWorkingDir);
  REGISTER_HANDLER_EQUALS_1(QStartNoAckMode);
  REGISTER_HANDLER_EQUALS_1(QSyncThreadState);
  REGISTER_HANDLER_EQUALS_1(QThreadSuffixSupported);
  REGISTER_HANDLER_EQUALS_1(Qbtrace);
  REGISTER_HANDLER_EQUALS_1(qAttached);
  REGISTER_HANDLER_EQUALS_1(qC);
  REGISTER_HANDLER_EQUALS_1(qCRC);
  REGISTER_HANDLER_EQUALS_1(qEcho);
  REGISTER_HANDLER_EQUALS_1(qFileLoadAddress);
  REGISTER_HANDLER_EQUALS_1(qGDBServerVersion);
  REGISTER_HANDLER_EQUALS_1(qGetPid);
  REGISTER_HANDLER_EQUALS_1(qGetProfileData);
  REGISTER_HANDLER_EQUALS_1(qGetTIBAddr);
  REGISTER_HANDLER_EQUALS_1(qGetTLSAddr);
  REGISTER_HANDLER_EQUALS_1(qGetWorkingDir);
  REGISTER_HANDLER_EQUALS_1(qGroupName);
  REGISTER_HANDLER_EQUALS_1(qHostInfo);
  REGISTER_HANDLER_EQUALS_1(qKillSpawnedProcess);
  REGISTER_HANDLER_EQUALS_1(qL);
  REGISTER_HANDLER_EQUALS_1(qLaunchGDBServer);
  REGISTER_HANDLER_EQUALS_1(qLaunchSuccess);
  REGISTER_HANDLER_EQUALS_1(qMemoryRegionInfo);
  REGISTER_HANDLER_EQUALS_1(qModuleInfo);
  REGISTER_HANDLER_EQUALS_1(qOffsets);
  REGISTER_HANDLER_EQUALS_1(qP);
  REGISTER_HANDLER_EQUALS_1(qPlatform_chmod);
  REGISTER_HANDLER_EQUALS_1(qPlatform_mkdir);
  REGISTER_HANDLER_EQUALS_1(qPlatform_shell);
  REGISTER_HANDLER_EQUALS_1(qProcessInfo);
  REGISTER_HANDLER_EQUALS_1(qProcessInfoPID);
  REGISTER_HANDLER_EQUALS_1(qRcmd);
  REGISTER_HANDLER_STARTS_WITH_1(qRegisterInfo);
  REGISTER_HANDLER_EQUALS_1(qSearch);
  REGISTER_HANDLER_EQUALS_1(qShlibInfoAddr);
  REGISTER_HANDLER_EQUALS_1(qSpeedTest);
  REGISTER_HANDLER_EQUALS_1(qStepPacketSupported);
  REGISTER_HANDLER_EQUALS_1(qSupported);
  REGISTER_HANDLER_EQUALS_1(qSupportsDetachAndStayStopped);
  REGISTER_HANDLER_EQUALS_1(qSymbol);
  REGISTER_HANDLER_STARTS_WITH_1(qThreadStopInfo);
  REGISTER_HANDLER_EQUALS_1(qThreadExtraInfo);
  REGISTER_HANDLER_EQUALS_1(qTStatus);
  REGISTER_HANDLER_EQUALS_1(qUserName);
  REGISTER_HANDLER_EQUALS_1(qVAttachOrWaitSupported);
  REGISTER_HANDLER_EQUALS_1(qWatchpointSupportInfo);
  REGISTER_HANDLER_EQUALS_1(qXfer);
  REGISTER_HANDLER_EQUALS_1(qfProcessInfo);
  REGISTER_HANDLER_EQUALS_1(qsProcessInfo);
  REGISTER_HANDLER_EQUALS_1(qfThreadInfo);
  REGISTER_HANDLER_EQUALS_1(qsThreadInfo);
  REGISTER_HANDLER_EQUALS_1(R);
  REGISTER_HANDLER_EQUALS_1(r);
  REGISTER_HANDLER_EQUALS_1(S);
  REGISTER_HANDLER_EQUALS_1(s);
  REGISTER_HANDLER_EQUALS_1(T);
  REGISTER_HANDLER_EQUALS_1(t);
  REGISTER_HANDLER_EQUALS_1(vAttach);
  REGISTER_HANDLER_EQUALS_1(vAttachName);
  REGISTER_HANDLER_EQUALS_1(vAttachWait);
  REGISTER_HANDLER_EQUALS_1(vAttachOrWait);
  REGISTER_HANDLER_EQUALS_2("vCont?", vContQuestionMark);
  REGISTER_HANDLER_EQUALS_1(vCont);
  REGISTER_HANDLER_EQUALS_1(vFile);
  REGISTER_HANDLER_EQUALS_1(vFlashDone);
  REGISTER_HANDLER_EQUALS_1(vFlashErase);
  REGISTER_HANDLER_EQUALS_1(vFlashWrite);
  REGISTER_HANDLER_EQUALS_1(vKill);
  REGISTER_HANDLER_EQUALS_1(vRun);
  REGISTER_HANDLER_EQUALS_1(vStopped);
  REGISTER_HANDLER_EQUALS_1(X);
  REGISTER_HANDLER_EQUALS_1(x);
  REGISTER_HANDLER_EQUALS_1(Z);
  REGISTER_HANDLER_EQUALS_1(z);

#undef REGISTER_HANDLER_STARTS_WITH_1
#undef REGISTER_HANDLER_STARTS_WITH_2
#undef REGISTER_HANDLER_EQUALS_1
#undef REGISTER_HANDLER_EQUALS_2
}

bool Session::ParseList(std::string const &string, char separator,
                        std::function<void(std::string const &)> const &cb) {
  if (string.empty())
    return false;

  size_t first = 0, end = string.size(), last = end;
  while (first < end) {
    last = string.find(separator, first);
    if (last == std::string::npos) {
      cb(string.substr(first));
      break;
    } else {
      cb(string.substr(first, last - first));
      first = last + 1;
    }
  }

  return true;
}

//
// Parses and formats the brain-dead idea of hex values
// in native-endian format used by GDB remote protocol
// to native and back.
//
bool Session::parseAddress(ds2::Address &address, const char *ptr, char **eptr,
                           Endian endianness) const {
  DS2ASSERT(ptr != nullptr);
  DS2ASSERT(eptr != nullptr);
  DS2ASSERT(endianness == kEndianBig || endianness == kEndianLittle); // No PDP.

  uint64_t value = strtoull(ptr, eptr, 16);
  // If *eptr contains the original value of ptr, there was nothing read.
  if (ptr == *eptr)
    return false;

  if (endianness == kEndianLittle) {
    size_t regsize = _delegate->getGPRSize();
    value = Swap64(value) >> (64 - regsize);
    if (regsize != 64) {
      value &= (1ULL << regsize) - 1;
    }
  }

  address = value;
  return true;
}

std::string Session::formatAddress(Address const &address,
                                   Endian endianness) const {
  DS2ASSERT(endianness == kEndianBig || endianness == kEndianLittle); // No PDP.

  std::ostringstream ss;
  uint64_t value = address;
  size_t regsize = _delegate->getGPRSize();

  if (regsize != 64) {
    value &= (1ULL << regsize) - 1;
  }

  if (endianness == kEndianLittle) {
    value = Swap64(value) >> (64 - regsize);
  }

  ss << std::hex << std::setw(regsize >> 2) << std::setfill('0') << value;
  return ss.str();
}

OpenFlags Session::ConvertOpenFlags(uint32_t protocolFlags) {
  int flags = 0;
  if (_compatMode == kCompatibilityModeLLDB) {
    if (protocolFlags & (1u << 0)) // eOpenOptionRead
      flags |= kOpenFlagRead;
    if (protocolFlags & (1u << 1)) // eOpenOptionWrite
      flags |= kOpenFlagWrite;
    if (flags == 0)
      return kOpenFlagInvalid;

    if (protocolFlags & (1u << 2)) // eOpenOptionAppend
      flags |= kOpenFlagAppend;
    if (protocolFlags & (1u << 3)) // eOpenOptionTruncate
      flags |= kOpenFlagTruncate;
    if (protocolFlags & (1u << 4)) // eOpenOptionNonBlocking
      flags |= kOpenFlagNonBlocking;
    if (protocolFlags & (1u << 5)) // eOpenOptionCanCreate
      flags |= kOpenFlagCreate;
    if (protocolFlags & (1u << 6)) // eOpenOptionCanCreateNewOnly
      flags |= (kOpenFlagCreate | kOpenFlagNewOnly);
    if (protocolFlags & (1u << 7)) // eOpenOptionDontFollowSymlinks
      flags |= kOpenFlagNoFollow;
    if (protocolFlags & (1u << 8)) // eOpenOptionCloseOnExec
      flags |= kOpenFlagCloseOnExec;
  } else {
    if ((protocolFlags & 0x3) == 0x0) // O_RDONLY
      flags |= kOpenFlagRead;
    else if (protocolFlags & 0x1) // O_WRONLY
      flags |= kOpenFlagWrite;
    else if (protocolFlags & 0x2) // O_RDWR
      flags |= (kOpenFlagRead | kOpenFlagWrite);
    else
      return kOpenFlagInvalid; // Invalid mode

    if (protocolFlags & 0x8) // O_APPEND
      flags |= kOpenFlagAppend;
    if (protocolFlags & 0x200) // O_CREAT
      flags |= kOpenFlagCreate;
    if (protocolFlags & 0x400) // O_TRUNC
      flags |= kOpenFlagTruncate;
    if (protocolFlags & 0x800) // O_EXCL
      flags |= kOpenFlagNewOnly;
  }

  return static_cast<OpenFlags>(flags);
}

//
// Packet:        \x03
// Description:   A Ctrl+C has been issued in the debugger.
// Compatibility: GDB, LLDB
//
void Session::Handle_ControlC(ProtocolInterpreter::Handler const &,
                              std::string const &) {
  // The delegate should issue a signal to the target process,
  // as such the stop code is returned when the signal has been
  // delivered as part of the waiting process.
  CHK_SEND(_delegate->onInterrupt(*this));
}

//
// Packet:        !
// Description:   Enable Extended Mode
// Compatibility: GDB
//
void Session::Handle_ExclamationMark(ProtocolInterpreter::Handler const &,
                                     std::string const &) {
  sendError(_delegate->onEnableExtendedMode(*this));
}

//
// Packet:        ?
// Description:   Get target stop reason
// Compatibility: GDB, LLDB
//
void Session::Handle_QuestionMark(ProtocolInterpreter::Handler const &,
                                  std::string const &) {
  StopInfo stop;
  CHK_SEND(_delegate->onQueryThreadStopInfo(*this, ProcessThreadId(), stop));

  send(stop.encode(_compatMode, _threadsInStopReply));

  if (_compatMode != kCompatibilityModeLLDB) {
    //
    // Update the 'c' and 'g' ptids.
    //
    _ptids['c'] = _ptids['g'] = stop.ptid;
  }
}

//
// Packet:        A arglen,argnum,arg[,arg[,...]]
// Description:   Program Arguments
// Compatibility: GDB, LLDB
//
void Session::Handle_A(ProtocolInterpreter::Handler const &,
                       std::string const &args) {
  std::map<size_t, std::string> argmap;

  char *eptr = const_cast<char *>(args.c_str());
  char *end = eptr + args.length();

  while (eptr < end && *eptr != '\0') {
    if (!argmap.empty()) {
      if (*eptr++ != ',') {
        sendError(kErrorInvalidArgument);
        return;
      }
    }

    uint32_t nchars = std::strtoul(eptr, &eptr, 10);
    if (*eptr++ != ',') {
      sendError(kErrorInvalidArgument);
      return;
    }
    uint32_t argno = std::strtoul(eptr, &eptr, 10);
    if (*eptr++ != ',') {
      sendError(kErrorInvalidArgument);
      return;
    }

    std::string &s = argmap[argno];
    for (size_t n = 0; n < nchars; n += 2) {
      s += static_cast<char>(HexToByte(eptr + n));
    }
    eptr += nchars;
  }

  StringCollection arguments;

  if (!argmap.empty()) {
    arguments.resize(argmap.rbegin()->first + 1);
    for (auto const &it : argmap) {
      arguments[it.first] = it.second;
    }
  }

  sendError(_delegate->onSetProgramArguments(*this, arguments));
}

//
// Packet:        B addr,mode
// Description:   Sets or clears a breakpoint at the address specified.
// Compatibility: GDB
//
void Session::Handle_B(ProtocolInterpreter::Handler const &,
                       std::string const &args) {
  char *eptr;
  uint64_t address = strtoull(args.c_str(), &eptr, 16);
  if (*eptr++ != ',') {
    sendError(kErrorInvalidArgument);
    return;
  }

  ErrorCode error;
  switch (*eptr) {
  case 'C':
    error =
        _delegate->onRemoveBreakpoint(*this, kSoftwareBreakpoint, address, 0);
    break;

  case 'S':
    error = _delegate->onInsertBreakpoint(*this, kSoftwareBreakpoint, address,
                                          0, StringCollection(),
                                          StringCollection(), false);
    break;

  default:
    error = kErrorInvalidArgument;
    break;
  }

  sendError(error);
}

//
// Packet:        b baud
// Description:   Set Serial Line speed
// Compatibility: GDB
//
void Session::Handle_b(ProtocolInterpreter::Handler const &,
                       std::string const &args) {
  uint32_t speed = std::strtoul(args.c_str(), nullptr, 0); // TODO maybe hex?
  sendError(_delegate->onSetBaudRate(*this, speed));
}

//
// Packet:        bc
// Description:   Backward continue
// Compatibility: GDB
//
void Session::Handle_bc(ProtocolInterpreter::Handler const &,
                        std::string const &args) {
  ThreadResumeAction action;
  action.action = kResumeActionBackwardContinue;

  ThreadResumeAction::Collection actions;
  actions.push_back(action);

  StopInfo stop;
  CHK_SEND(_delegate->onResume(*this, actions, stop));

  send(stop.encode(_compatMode, _threadsInStopReply));

  if (_compatMode != kCompatibilityModeLLDB) {
    //
    // Update the 'c' and 'g' ptids.
    //
    _ptids['c'] = _ptids['g'] = stop.ptid;
  }
}

//
// Packet:        bs
// Description:   Backward step
// Compatibility: GDB
//
void Session::Handle_bs(ProtocolInterpreter::Handler const &,
                        std::string const &args) {
  ThreadResumeAction action;
  action.action = kResumeActionBackwardStep;

  ThreadResumeAction::Collection actions;
  actions.push_back(action);

  StopInfo stop;
  CHK_SEND(_delegate->onResume(*this, actions, stop));

  send(stop.encode(_compatMode, _threadsInStopReply));

  if (_compatMode != kCompatibilityModeLLDB) {
    //
    // Update the 'c' and 'g' ptids.
    //
    _ptids['c'] = _ptids['g'] = stop.ptid;
  }
}

//
// Packet:        C sig[;addr][;thread:tid]
// Description:   Continue at the address specified if any, using
//                the specified signal.
// Compatibility: GDB, LLDB
//
void Session::Handle_C(ProtocolInterpreter::Handler const &,
                       std::string const &args) {
  ProcessThreadId ptid;
  int signal;
  Address address;
  char *bptr;
  char *eptr;

  signal = std::strtol(args.c_str(), &bptr, 16);

  if (*bptr == ';' && parseAddress(address, bptr + 1, &eptr, kEndianNative)) {
    bptr = eptr;
  }

  if (_compatMode == kCompatibilityModeLLDB) {
    //
    // LLDB will send ;thread:tid when sending this command; pid
    // is deduced from current process.
    //
    if (*bptr++ == ';') {
      if (!ptid.parse(bptr, kCompatibilityModeLLDBThread)) {
        sendError(kErrorInvalidArgument);
        return;
      }
    }
  } else {
    //
    // Use what previously has been set with H packet.
    //
    ptid = _ptids['c'];
  }

  ThreadResumeAction action;
  action.action = kResumeActionContinueWithSignal;
  action.ptid = ptid;
  action.signal = signal;
  action.address = address;

  ThreadResumeAction::Collection actions;
  actions.push_back(action);

  StopInfo stop;
  CHK_SEND(_delegate->onResume(*this, actions, stop));

  send(stop.encode(_compatMode, _threadsInStopReply));

  if (_compatMode != kCompatibilityModeLLDB) {
    //
    // Update the 'c' and 'g' ptids.
    //
    _ptids['c'] = _ptids['g'] = stop.ptid;
  }
}

//
// Packet:        c [addr][;thread:tid]
// Description:   Continue at the address specified if any.
// Compatibility: GDB, LLDB
//
void Session::Handle_c(ProtocolInterpreter::Handler const &,
                       std::string const &args) {
  ProcessThreadId ptid;
  Address address;
  char *eptr;

  parseAddress(address, args.c_str(), &eptr, kEndianNative);

  if (_compatMode == kCompatibilityModeLLDB) {
    //
    // LLDB will send ;thread:tid when sending this command; pid
    // is deduced from current process.
    //
    if (*eptr++ == ';') {
      if (!ptid.parse(eptr, kCompatibilityModeLLDBThread)) {
        sendError(kErrorInvalidArgument);
        return;
      }
    }
  } else {
    //
    // Use what previously has been set with H packet.
    //
    ptid = _ptids['c'];
  }

  ThreadResumeAction action;
  action.action = kResumeActionContinue;
  action.ptid = ptid;
  action.address = address;

  ThreadResumeAction::Collection actions;
  actions.push_back(action);

  StopInfo stop;
  CHK_SEND(_delegate->onResume(*this, actions, stop));

  send(stop.encode(_compatMode, _threadsInStopReply));

  if (_compatMode != kCompatibilityModeLLDB) {
    //
    // Update the 'c' and 'g' ptids.
    //
    _ptids['c'] = _ptids['g'] = stop.ptid;
  }
}

//
// Packet:        D[;pid]
//                D[1[;pid]] (LLDB Extension)
// Description:   Detach from the target
// Compatibility: GDB, LLDB
//
void Session::Handle_D(ProtocolInterpreter::Handler const &,
                       std::string const &args) {
  char *eptr;
  uint32_t mode = 0;
  ProcessId pid = kAnyProcessId;

  if (!args.empty()) {
    mode = std::strtoul(args.c_str(), &eptr, 10);
    if (*eptr++ == ';') {
      pid = std::strtoul(eptr, nullptr, 16);
    }
  }

  sendError(_delegate->onDetach(*this, pid, mode == 1));
}

//
// Packet:        d
// Description:   Toggle Debug flag
// Compatibility: GDB
//
void Session::Handle_d(ProtocolInterpreter::Handler const &,
                       std::string const &) {
  sendError(_delegate->onToggleDebugFlag(*this));
}

//
// Packet:        G[;thread:tid]
// Description:   Write general registers
// Compatibility: GDB, LLDB
//
void Session::Handle_G(ProtocolInterpreter::Handler const &,
                       std::string const &args) {
  ProcessThreadId ptid;

  if (_compatMode == kCompatibilityModeLLDB) {
    //
    // LLDB will send ;thread:tid when sending this command; pid
    // is deduced from current process.
    //
    if (!args.empty()) {
      if (args[0] == ';') {
        if (!ptid.parse(args.substr(1), kCompatibilityModeLLDBThread)) {
          sendError(kErrorInvalidArgument);
          return;
        }
      }
    }
  } else {
    //
    // Use what previously has been set with H packet.
    //
    ptid = _ptids['g'];
  }

  std::vector<uint64_t> regs;
  size_t regsize = _delegate->getGPRSize() / 8; // Register size in bytes.
  size_t reglen = regsize * 2; // Two characters per byte in hex.
  size_t nargs = args.size() / reglen;

  for (size_t n = 0; n < nargs; n++) {
    Address address;
    std::string val = args.substr(n * reglen, reglen);
    char *eptr;

    parseAddress(address, val.c_str(), &eptr, kEndianNative);
    regs.push_back(address);
  }

  sendError(_delegate->onWriteGeneralRegisters(*this, ptid, regs));
}

//
// Packet:        g[;thread:tid]
// Description:   Read general registers
// Compatibility: GDB, LLDB
//
void Session::Handle_g(ProtocolInterpreter::Handler const &,
                       std::string const &args) {
  Architecture::GPRegisterValueVector regs;
  ProcessThreadId ptid;

  if (_compatMode == kCompatibilityModeLLDB) {
    //
    // LLDB will send ;thread:tid when sending this command; pid
    // is deduced from current process.
    //
    if (!args.empty()) {
      if (args[0] == ';') {
        if (!ptid.parse(args.substr(1), kCompatibilityModeLLDBThread)) {
          sendError(kErrorInvalidArgument);
          return;
        }
      }
    }
  } else {
    //
    // Use what previously has been set with H packet.
    //
    ptid = _ptids['g'];
  }

  CHK_SEND(_delegate->onReadGeneralRegisters(*this, ptid, regs));

  std::ostringstream ss;
  for (auto reg : regs) {
    ss << formatAddress(reg.value, kEndianNative);
  }
  send(ss.str());
}

//
// Packet:        H op thread-id
// Description:   Sets the thread-id for the subsequent operation
//                specified by op.
// Compatibility: GDB, LLDB
//
void Session::Handle_H(ProtocolInterpreter::Handler const &,
                       std::string const &args) {
  if (args.length() < 2) {
    sendError(kErrorInvalidArgument);
    return;
  }

  ProcessThreadId ptid;
  uint8_t command = args[0];

  if (!ptid.parse(args.substr(1), _compatMode)) {
    sendError(kErrorInvalidArgument);
    return;
  }

  //
  // It is an error to specify p-1.tid, so we emit an error in such case.
  //
  if (ptid.pid == kAllProcessId) {
    sendError(kErrorInvalidArgument);
    return;
  }

  //
  // Query if the thread is alive before proceeding.
  //
  CHK_SEND(_delegate->onThreadIsAlive(*this, ptid));

  _ptids[command] = ptid;
  sendOK();

  DS2LOG(Debug, "setting command '%c' to pid %" PRIu64 " tid %" PRIu64, command,
         (uint64_t)ptid.pid, (uint64_t)ptid.tid);
}

//
// NOTE: This packet has completely different behavior in LLDB and GDB
//
// Packet:        I sig[;addr[,nnn]]
// Description:   Step the remote target by a single clock cycle
//                at the address specified if any, using the
//                specified signal.
// Compatibility: GDB
//
// Packet:        I data
// Description:   Send data to the inferior process. The data
//                is hex-encoded.
// Compatibility: LLDB
//
void Session::Handle_I(ProtocolInterpreter::Handler const &,
                       std::string const &args) {
  if (_compatMode == kCompatibilityModeLLDB) {
    ByteVector data(HexToByteVector(args.data()));
    CHK_SEND(_delegate->onSendInput(*this, data));

    sendOK();
  } else {
    char *eptr;
    int signal;
    Address address;
    uint32_t ncycles = 1;

    signal = std::strtol(args.c_str(), &eptr, 16);
    if (*eptr++ == ';') {
      parseAddress(address, eptr, &eptr, kEndianNative);
      if (*eptr++ == ',') {
        ncycles = std::strtoul(eptr, nullptr, 16); // TODO is it really hex?
      }
    }

    //
    // Use what previously has been set with H packet.
    //
    ThreadResumeAction action;
    action.action = kResumeActionSingleStepCycleWithSignal;
    action.signal = signal;
    action.address = address;
    action.ncycles = ncycles;

    ThreadResumeAction::Collection actions;
    actions.push_back(action);

    StopInfo stop;
    CHK_SEND(_delegate->onResume(*this, actions, stop));

    send(stop.encode(_compatMode, _threadsInStopReply));

    //
    // Update the 'c' and 'g' ptids.
    //
    _ptids['c'] = _ptids['g'] = stop.ptid;
  }
}

//
// Packet:        i [addr[,nnn]]
// Description:   Step the remote target by a single clock cycle
//                at the address specified if any.
// Compatibility: GDB
//
void Session::Handle_i(ProtocolInterpreter::Handler const &,
                       std::string const &args) {
  char *eptr;
  Address address;
  uint32_t ncycles = 1;

  parseAddress(address, args.c_str(), &eptr, kEndianNative);
  if (*eptr++ == ',') {
    ncycles = std::strtoul(eptr, nullptr, 16); // TODO is it really hex?
  }

  //
  // Use what previously has been set with H packet.
  //
  ThreadResumeAction action;
  action.action = kResumeActionSingleStepCycle;
  action.address = address;
  action.ncycles = ncycles;

  ThreadResumeAction::Collection actions;
  actions.push_back(action);

  StopInfo stop;
  CHK_SEND(_delegate->onResume(*this, actions, stop));

  send(stop.encode(_compatMode, _threadsInStopReply));

  if (_compatMode != kCompatibilityModeLLDB) {
    //
    // Update the 'c' and 'g' ptids.
    //
    _ptids['c'] = _ptids['g'] = stop.ptid;
  }
}

//
// Packet:        jThreadsInfo
// Description:   Get information on all threads at once
// Compatibility: LLDB
//
void Session::Handle_jThreadsInfo(ProtocolInterpreter::Handler const &,
                                  std::string const &) {
  StopInfo processStop;
  std::vector<StopInfo> stops;
  CHK_SEND(_delegate->fetchStopInfoForAllThreads(*this, stops, processStop));

  if (_compatMode != kCompatibilityModeLLDB) {
    //
    // Update the 'c' and 'g' ptids.
    //
    _ptids['c'] = _ptids['g'] = processStop.ptid;
  }

  JSArray jsonObj;
  for (auto const &stop : stops) {
    jsonObj.append(stop.encodeJson());
  }

  send(jsonObj.toString(), false);
}

//
// Packet:        k
// Description:   Kill target
// Compatibility: GDB, LLDB
//
void Session::Handle_k(ProtocolInterpreter::Handler const &,
                       std::string const &) {
  StopInfo stop;
  CHK_SEND(_delegate->onTerminate(*this, ProcessThreadId(), stop));

  send(stop.encode(_compatMode, _threadsInStopReply));

  if (_compatMode != kCompatibilityModeLLDB) {
    //
    // Update the 'c' and 'g' ptids.
    //
    _ptids['c'] = _ptids['g'] = stop.ptid;
  }
}

//
// Packet:        _M size,permissions
// Description:   Allocate memory in the remote target process.
// Compatibility: LLDB
//
void Session::Handle__M(ProtocolInterpreter::Handler const &,
                        std::string const &args) {
  char *eptr;
  uint64_t length;
  uint32_t protection = 0;

  length = strtoull(args.c_str(), &eptr, 16);
  if (*eptr++ != ',') {
    sendError(kErrorInvalidArgument);
    return;
  }
  for (; *eptr != '\0'; eptr++) {
    switch (*eptr) {
    case 'r':
      protection |= kProtectionRead;
      break;
    case 'w':
      protection |= kProtectionWrite;
      break;
    case 'x':
      protection |= kProtectionExecute;
      break;
    default:
      break;
    }
  }

  Address address;
  CHK_SEND(_delegate->onAllocateMemory(*this, length, protection, address));

  std::ostringstream ss;
  ss << std::hex << std::setfill('0') << std::setw(_delegate->getGPRSize() >> 2)
     << address.value();
  send(ss.str());
}

//
// Packet:        _m addr
// Description:   Deallocate memory in the remote target process
//                previously allocated at the address specified.
// Compatibility: LLDB
//
void Session::Handle__m(ProtocolInterpreter::Handler const &,
                        std::string const &args) {
  uint64_t address = strtoull(args.c_str(), nullptr, 16);
  sendError(_delegate->onDeallocateMemory(*this, address));
}

//
// Packet:        M addr,length:XX...
// Description:   Write to target memory, data is hex encoded.
// Compatibility: GDB, LLDB
//
void Session::Handle_M(ProtocolInterpreter::Handler const &,
                       std::string const &args) {
  char *eptr;
  uint64_t address;
  uint64_t length;

  address = strtoull(args.c_str(), &eptr, 16);
  if (*eptr++ != ',') {
    sendError(kErrorInvalidArgument);
    return;
  }
  length = strtoull(eptr, &eptr, 16);
  if (*eptr++ != ':') {
    sendError(kErrorInvalidArgument);
    return;
  }

  ByteVector data(HexToByteVector(eptr));
  if (data.size() > length) {
    data.resize(length);
  }

  size_t nwritten = 0;
  sendError(_delegate->onWriteMemory(*this, address, data, nwritten));
}

//
// Packet:        m addr,length
// Description:   Read from target memory
// Compatibility: GDB, LLDB
//
void Session::Handle_m(ProtocolInterpreter::Handler const &,
                       std::string const &args) {
  uint64_t address;
  uint64_t length;
  ByteVector data;
  char *eptr;

  address = strtoull(args.c_str(), &eptr, 16);
  if (*eptr++ != ',') {
    sendError(kErrorInvalidArgument); // Uhoh
    return;
  }
  length = strtoull(eptr, nullptr, 16);

  CHK_SEND(_delegate->onReadMemory(*this, address, length, data));

  send(ToHex(data));
}

//
// Packet:        P n=r
// Description:   Write register n value r
// Compatibility: GDB, LLDB
//
void Session::Handle_P(ProtocolInterpreter::Handler const &,
                       std::string const &args) {
  char *eptr;
  char *ptidptr;
  ProcessThreadId ptid;
  std::string value;
  uint32_t regno;

  regno = std::strtoul(args.c_str(), &eptr, 16);
  if (*eptr++ != '=') {
    sendError(kErrorInvalidArgument);
    return;
  }

  ptidptr = std::strchr(eptr, ';');

  if (_compatMode != kCompatibilityModeLLDB || ptidptr == nullptr) {
    ptidptr = std::strchr(eptr, '\0');
  }

  value = HexToString(std::string(eptr, ptidptr - eptr));

  if (_compatMode == kCompatibilityModeLLDB) {
    //
    // LLDB will send ;thread:tid when sending 'P' commands; pid
    // is deduced from current process.
    //
    if (*ptidptr++ == ';') {
      if (!ptid.parse(ptidptr, kCompatibilityModeLLDBThread)) {
        sendError(kErrorInvalidArgument);
        return;
      }
    }
  } else {
    //
    // Use what previously has been set with H packet for the 'g' packet.
    //
    ptid = _ptids['g'];
  }

  sendError(_delegate->onWriteRegisterValue(*this, ptid, regno, value));
}

//
// Packet:        p n
// Description:   Read register n value
// Compatibility: GDB, LLDB
//
void Session::Handle_p(ProtocolInterpreter::Handler const &,
                       std::string const &args) {
  char *eptr;
  ProcessThreadId ptid;
  uint32_t regno = std::strtoul(args.c_str(), &eptr, 16);

  if (_compatMode == kCompatibilityModeLLDB) {
    //
    // LLDB will send ;thread:tid when sending 'p' commands; pid
    // is deduced from current process.
    //
    if (*eptr++ == ';') {
      if (!ptid.parse(eptr, kCompatibilityModeLLDBThread)) {
        sendError(kErrorInvalidArgument);
        return;
      }
    }
  } else {
    //
    // Use what previously has been set with H packet for the 'g' packet.
    //
    ptid = _ptids['g'];
  }

  std::string value;
  CHK_SEND(_delegate->onReadRegisterValue(*this, ptid, regno, value));

  send(ToHex(value));
}

//
// Packet:        QEnvironment:name=value
// Description:   Sets the environment variable to the value specified
// Compatibility: LLDB
//
void Session::Handle_QEnvironment(ProtocolInterpreter::Handler const &,
                                  std::string const &args) {
  std::string key, value;

  size_t eq = args.find('=');
  if (eq != std::string::npos) {
    key = args.substr(0, eq);
    value = args.substr(eq + 1);
  }

  sendError(_delegate->onSetEnvironmentVariable(*this, key, value));
}

//
// Packet:        QEnvironmentHexEncoded:name=value
// Description:   Sets the environment variable to the value specified
// Compatibility: LLDB
//
void Session::Handle_QEnvironmentHexEncoded(
    ProtocolInterpreter::Handler const &, std::string const &args) {
  std::string key, value, ev(HexToString(args));

  size_t eq = ev.find('=');
  if (eq != std::string::npos) {
    key = ev.substr(0, eq);
    value = ev.substr(eq + 1);
  }

  sendError(_delegate->onSetEnvironmentVariable(*this, key, value));
}

//
// Packet:        QNonStop:bool
// Description:   Enter or exit non-stop mode.
// Compatibility: GDB
//
void Session::Handle_QNonStop(ProtocolInterpreter::Handler const &,
                              std::string const &args) {
  sendError(_delegate->onNonStopMode(*this, std::atoi(args.c_str()) != 0));
}

//
// Packet:        QPassSignals:signal[;signal]...
// Description:   Each listed signal should be passed directly to the
//                inferior process.
// Compatibility: GDB
//
void Session::Handle_QPassSignals(ProtocolInterpreter::Handler const &,
                                  std::string const &args) {
  std::vector<int> signals;
  ParseList(args, ';', [&](std::string const &arg) {
    signals.push_back(std::strtoul(arg.c_str(), nullptr, 16));
  });

  sendError(_delegate->onPassSignals(*this, signals));
}

//
// Packet:        QProgramSignals:signal[;signal]...
// Description:   Each listed signal may be delivered to the
//                inferior process.
// Compatibility: GDB
//
void Session::Handle_QProgramSignals(ProtocolInterpreter::Handler const &,
                                     std::string const &args) {
  std::vector<int> signals;
  ParseList(args, ';', [&](std::string const &arg) {
    signals.push_back(std::strtoul(arg.c_str(), nullptr, 16));
  });

  sendError(_delegate->onProgramSignals(*this, signals));
}

//
// Packet:        QSaveRegisterState [thread:XXXX]
// Description:   Save all registers and return a non-zero unique
//                integer ID that represents saved registers.
// Compatibility: LLDB
//
void Session::Handle_QSaveRegisterState(ProtocolInterpreter::Handler const &,
                                        std::string const &args) {
  char const *eptr = args.c_str();
  ProcessThreadId ptid;
  uint64_t id;

  ptid.parse(eptr, kCompatibilityModeLLDBThread);

  CHK_SEND(_delegate->onSaveRegisters(*this, ptid, id));

  std::ostringstream ss;
  ss << id;
  send(ss.str());
}

//
// Packet:        QRestoreRegisterState saveId[;thread:XXXX]
// Description:   Save all registers and return a non-zero unique
//                integer ID that represents saved registers.
// Compatibility: LLDB
//
void Session::Handle_QRestoreRegisterState(ProtocolInterpreter::Handler const &,
                                           std::string const &args) {
  char const *eptr = args.c_str();
  ProcessThreadId ptid;
  uint64_t id;

  id = strtoull(eptr, const_cast<char **>(&eptr), 10);

  if (*eptr++ == ';')
    ptid.parse(eptr, kCompatibilityModeLLDBThread);

  sendError(_delegate->onRestoreRegisters(*this, ptid, id));
}

//
// Packet:        QStartNoAckMode
// Description:   Request that the remote stub disable the normal
//                protocol acknowledgment.
// Compatibility: GDB, LLDB
//
void Session::Handle_QStartNoAckMode(ProtocolInterpreter::Handler const &,
                                     std::string const &) {
  setAckMode(false);
  sendOK();
}

//
// Packet:        QSyncThreadState
// Description:   Do whatever necessary to synchronize the thread
//                information about the debugged process.
// Compatibility: LLDB
//
void Session::Handle_QSyncThreadState(ProtocolInterpreter::Handler const &,
                                      std::string const &) {
  sendError(_delegate->onSynchronizeThreadState(*this, kAnyProcessId));
}

//
// Packet:        QAllow:op:value[;op:value[;...]]
// Description:   Specify which operations the debugger expects to
//                request of the target.
// Compatibility: GDB
//
void Session::Handle_QAllow(ProtocolInterpreter::Handler const &,
                            std::string const &args) {
  std::map<std::string, bool> ops;

  ParseList(args, ';', [&](std::string const &arg) {
    size_t colon = arg.find(':');
    if (colon != std::string::npos) {
      ops[arg.substr(0, colon)] =
          std::strtoul(&arg[colon + 1], nullptr, 10) != 0;
    }
  });

  sendError(_delegate->onAllowOperations(*this, ops));
}

//
// Packet:        QAgent:value
// Description:   Turns on or off the control agent.
// Compatibility: GDB
//
void Session::Handle_QAgent(ProtocolInterpreter::Handler const &,
                            std::string const &args) {
  uint32_t value = std::strtoul(args.c_str(), nullptr, 16);
  sendError(_delegate->onEnableControlAgent(*this, value != 0));
}

//
// Packet:        Qbtrace:[bts|off]
// Description:   Enable branch tracing for the current thread using
//                bts tracing
// Compatibility: GDB
//
void Session::Handle_Qbtrace(ProtocolInterpreter::Handler const &,
                             std::string const &args) {
  bool enabled;
  if (args == "bts") {
    enabled = true;
  } else if (args == "off") {
    enabled = false;
  } else {
    sendError(kErrorInvalidArgument);
    return;
  }

  sendError(_delegate->onEnableBTSTracing(*this, enabled));
}

//
// Packet:        QDisableRandomization:value
// Description:   Disable Address Space Layout Randomization
// Compatibility: GDB
//
void Session::Handle_QDisableRandomization(ProtocolInterpreter::Handler const &,
                                           std::string const &args) {
  uint32_t value = std::strtoul(args.c_str(), nullptr, 16);
  sendError(_delegate->onDisableASLR(*this, value != 0));
}

//
// Packet:        QSetMaxPacketSize:size
// Description:   Tell the debug server the max sized packet the
//                debugger can handle.
// Compatibility: LLDB
//
void Session::Handle_QSetMaxPacketSize(ProtocolInterpreter::Handler const &,
                                       std::string const &args) {
  uint32_t size = std::strtoul(args.c_str(), nullptr, 16);
  if (size == 0) {
    sendError(kErrorInvalidArgument);
    return;
  }

  sendError(_delegate->onSetMaxPacketSize(*this, size));
}

//
// Packet:        QSetMaxPayloadSize:size
// Description:   Tell the debug server the max sized payload the
//                debugger can handle.
// Compatibility: LLDB
//
void Session::Handle_QSetMaxPayloadSize(ProtocolInterpreter::Handler const &,
                                        std::string const &args) {
  uint32_t size = std::strtoul(args.c_str(), nullptr, 16);
  if (size == 0) {
    sendError(kErrorInvalidArgument);
    return;
  }

  sendError(_delegate->onSetMaxPayloadSize(*this, size));
}

//
// Packet:        QSetSTDERR:path
// Description:   Redirect STDERR to the specified path.
// Compatibility: LLDB
//
void Session::Handle_QSetSTDERR(ProtocolInterpreter::Handler const &,
                                std::string const &args) {
  sendError(_delegate->onSetStdFile(*this, 2, HexToString(args)));
}

//
// Packet:        QSetSTDIN:path
// Description:   Redirect STDIN to the specified path.
// Compatibility: LLDB
//
void Session::Handle_QSetSTDIN(ProtocolInterpreter::Handler const &,
                               std::string const &args) {
  sendError(_delegate->onSetStdFile(*this, 0, HexToString(args)));
}

//
// Packet:        QSetSTDOUT:path
// Description:   Redirect STDOUT to the specified path.
// Compatibility: LLDB
//
void Session::Handle_QSetSTDOUT(ProtocolInterpreter::Handler const &,
                                std::string const &args) {
  sendError(_delegate->onSetStdFile(*this, 1, HexToString(args)));
}

//
// Packet:        QSetWorkingDir:path
// Description:   Sets the process working directory.
// Compatibility: LLDB
//
void Session::Handle_QSetWorkingDir(ProtocolInterpreter::Handler const &,
                                    std::string const &args) {
  sendError(_delegate->onSetWorkingDirectory(*this, HexToString(args)));
}

//
// Packet:        QLaunchArch:architecture
// Description:   Set the architecture to use when launching a process for
//                hosts that can run multiple architecture slices from
//                universal files.
// Compatibility: LLDB
//
void Session::Handle_QLaunchArch(ProtocolInterpreter::Handler const &,
                                 std::string const &args) {
  sendError(_delegate->onSetArchitecture(*this, args));
}

//
// Packet:        QListThreadsInStopReply
// Description:   Lists the threads of the process in the stop reply packet.
// Compatibility: LLDB
//
void Session::Handle_QListThreadsInStopReply(
    ProtocolInterpreter::Handler const &, std::string const &) {
  if (_compatMode != kCompatibilityModeLLDB) {
    DS2LOG(Debug, "entering LLDB compatibility mode");
    _compatMode = kCompatibilityModeLLDB;
  }

  _threadsInStopReply = true;
  sendOK();
}

//
// Packet:        QSetDisableASLR:value
// Description:   Disable Address Space Layout Randomization
// Compatibility: LLDB
//
void Session::Handle_QSetDisableASLR(ProtocolInterpreter::Handler const &,
                                     std::string const &args) {
  uint32_t value = std::strtoul(args.c_str(), nullptr, 10);
  sendError(_delegate->onDisableASLR(*this, value != 0));
}

//
// Packet:
// QSetEnableAsyncProfiling;[enable:[0|1]][;interval_usec:XXXXXX[;scan_type:0xYYYYYYY]
// Description:   Enable asynchronous profiling.
// Compatibility: LLDB
//
void Session::Handle_QSetEnableAsyncProfiling(
    ProtocolInterpreter::Handler const &, std::string const &args) {
  uint32_t scanType = 0;
  uint32_t interval = 0;
  bool enabled = false;

  ParseList(args, ';', [&](std::string const &arg) {
    if (arg.compare(0, 7, "enable:") == 0) {
      enabled = std::strtoul(&arg[7], nullptr, 0) != 0;
    } else if (arg.compare(0, 9, "scan_type:") == 0) {
      scanType = std::strtoul(&arg[9], nullptr, 0);
    } else if (arg.compare(0, 13, "interval_usec:") == 0) {
      interval = std::strtoul(&arg[13], nullptr, 0);
    }
  });

  sendError(_delegate->onEnableAsynchronousProfiling(
      *this, ProcessThreadId(), enabled, interval, scanType));
}

//
// Packet:
// QSetLogging:bitmask=LOG_A[|LOG_B[...]][;mode=[asl|file][;filename=[asl|path]]
// Description:   Set debug server logging
// Compatibility: LLDB
//
void Session::Handle_QSetLogging(ProtocolInterpreter::Handler const &,
                                 std::string const &args) {
  std::string mode;
  std::string filename;
  StringCollection flags;

  ParseList(args, ';', [&](std::string const &arg) {
    if (arg.compare(0, 8, "bitmask=") == 0) {
      ParseList(arg.substr(8), '|',
                [&](std::string const &flag) { flags.push_back(flag); });
    } else if (arg.compare(0, 5, "mode=") == 0) {
      mode = arg.substr(5);
    } else if (arg.compare(0, 9, "filename=") == 0) {
      filename = arg.substr(9);
    }
  });

  _delegate->onSetLogging(*this, mode, filename, flags);
}

//
// Packet:        QThreadSuffixSupported
// Description:   Try to enable thread suffix support for the 'g', 'G', 'p'
//                and 'P' packets
// Compatibility: LLDB
//
void Session::Handle_QThreadSuffixSupported(
    ProtocolInterpreter::Handler const &, std::string const &) {
  // We always support them.
  if (_compatMode != kCompatibilityModeLLDB) {
    DS2LOG(Debug, "entering LLDB compatibility mode");
    _compatMode = kCompatibilityModeLLDB;
  }

  sendOK();
}

//
// Packet:        qAttached:pid
// Description:   Return an indication of whether the remote server attached
//                to an existing process or created a new process
// Compatibility: GDB
//
void Session::Handle_qAttached(ProtocolInterpreter::Handler const &,
                               std::string const &args) {
  bool attached = false;
  ProcessId pid = strtoull(args.c_str(), nullptr, 16);
  CHK_SEND(_delegate->onQueryAttached(*this, pid, attached));

  send(attached ? "1" : "0");
}

//
// Packet:        qC
// Description:   Returns the current thread-id
// Compatibility: GDB, LLDB
//
void Session::Handle_qC(ProtocolInterpreter::Handler const &,
                        std::string const &) {
  ProcessThreadId ptid;
  CHK_SEND(_delegate->onQueryCurrentThread(*this, ptid));

  //
  // LLDB compatibility mode sends only the process id.
  //
  CompatibilityMode mode = _compatMode;
  if (mode == kCompatibilityModeLLDB) {
    mode = kCompatibilityModeGDB;
  }

  send("QC" + ptid.encode(mode));
}

//
// Packet:        qCRC addr,length
// Description:   Compute CRC of the target memory
// Compatibility: GDB
//
void Session::Handle_qCRC(ProtocolInterpreter::Handler const &,
                          std::string const &args) {
  uint64_t address;
  uint64_t length;
  std::string data;
  char *eptr;

  address = strtoull(args.c_str(), &eptr, 16);
  if (*eptr++ != ',') {
    sendError(kErrorInvalidArgument); // Uhoh
    return;
  }
  length = strtoull(eptr, nullptr, 16);

  uint32_t crc;
  CHK_SEND(_delegate->onComputeCRC(*this, address, length, crc));

  std::ostringstream ss;
  ss << std::hex << std::setw(8) << std::setfill('0') << crc;
  send(ss.str());
}

//
// Packet:        qEcho:%s
// Description:   %s is any valid string. The response simply echos the packet.
// Compatibility: LLDB
//
void Session::Handle_qEcho(ProtocolInterpreter::Handler const &,
                           std::string const &args) {
  send(args);
}

//
// Packet:        qFileLoadAddress:<file_path>
// Description:   Returns the load address of a memory mapped file.
// Compatibility: LLDB
//
void Session::Handle_qFileLoadAddress(ProtocolInterpreter::Handler const &,
                                      std::string const &args) {
  if (args.empty()) {
    sendError(kErrorInvalidArgument);
    return;
  }

  Address address;
  CHK_SEND(
      _delegate->onQueryFileLoadAddress(*this, HexToString(args), address));

  send(formatAddress(address, kEndianBig));
}

//
// Packet:        qGDBServerVersion
// Description:   Returns the remote server information.
// Compatibility: LLDB
//
void Session::Handle_qGDBServerVersion(ProtocolInterpreter::Handler const &,
                                       std::string const &) {
  ServerVersion version;
  CHK_SEND(_delegate->onQueryServerVersion(*this, version));

  send(version.encode());
}

//
// Packet:        qGetPid
// Description:   Query the current process debugging pid.
// Compatibility: LLDB
//
void Session::Handle_qGetPid(ProtocolInterpreter::Handler const &,
                             std::string const &) {
  ProcessThreadId ptid;
  CHK_SEND(_delegate->onQueryCurrentThread(*this, ptid));

  std::ostringstream ss;
  ss << std::hex << ptid.pid;
  send(ss.str());
}

//
// Packet:        qGetProfileData[;scan_type:0xYYYYYYY]
// Description:   Retrieves the profile data.
// Compatibility: LLDB
//
void Session::Handle_qGetProfileData(ProtocolInterpreter::Handler const &,
                                     std::string const &args) {
  uint32_t scanType = 0;

  ParseList(args, ';', [&](std::string const &arg) {
    if (arg.compare(0, 9, "scan_type:") == 0) {
      scanType = std::strtoul(&arg[9], nullptr, 0);
    }
  });

  void *TODO;
  CHK_SEND(
      _delegate->onQueryProfileData(*this, ProcessThreadId(), scanType, &TODO));

  // send(TODO.encode());
}

//
// Packet:        qGetTIBAddr:thread-id
// Description:   Query TIB address of the specified thread (Win32 only)
// Compatibility: GDB
//
void Session::Handle_qGetTIBAddr(ProtocolInterpreter::Handler const &,
                                 std::string const &args) {
  ProcessThreadId ptid;
  if (!ptid.parse(args, _compatMode)) {
    sendError(kErrorInvalidArgument);
    return;
  }

  Address address;
  CHK_SEND(_delegate->onQueryTIBAddress(*this, ptid, address));

  send(formatAddress(address, kEndianBig));
}

//
// Packet:        qGetTLSAddr:thread-id,offset,lm
// Description:   Query TLS address of the specified variable.
// Compatibility: GDB
//
void Session::Handle_qGetTLSAddr(ProtocolInterpreter::Handler const &,
                                 std::string const &args) {
  ProcessThreadId ptid;
  size_t comma = args.find(',');
  if (comma++ == std::string::npos || !ptid.parse(args, _compatMode)) {
    sendError(kErrorInvalidArgument);
    return;
  }

  char *eptr;
  uint64_t offset = strtoull(&args[comma], &eptr, 16);
  if (*eptr++ != ',') {
    sendError(kErrorInvalidArgument);
    return;
  }
  uint64_t linkMap = strtoull(eptr, &eptr, 16);

  Address address;
  CHK_SEND(_delegate->onQueryTLSAddress(*this, ptid, offset, linkMap, address));

  send(formatAddress(address, kEndianBig));
}

//
// Packet:        qGetWorkingDir
// Description:   Get the current working directory
// Compatibility: LLDB
void Session::Handle_qGetWorkingDir(ProtocolInterpreter::Handler const &,
                                    std::string const &args) {
  if (args.size() != 0) {
    sendError(kErrorInvalidArgument);
    return;
  }

  std::string workingDir;
  CHK_SEND(_delegate->onQueryWorkingDirectory(*this, workingDir));

  send(ToHex(workingDir));
}

//
// Packet:        qGroupName:group-id
// Description:   Query the group-id name
// Compatibility: LLDB
//
void Session::Handle_qGroupName(ProtocolInterpreter::Handler const &,
                                std::string const &args) {
  std::string name;
  UserId uid = UNPACK_ID(args.c_str());
  CHK_SEND(_delegate->onQueryGroupName(*this, uid, name));

  send(ToHex(name));
}

//
// Packet:        qHostInfo
// Description:   Query host information.
// Compatibility: LLDB
//
void Session::Handle_qHostInfo(ProtocolInterpreter::Handler const &,
                               std::string const &) {
  HostInfo info;
  CHK_SEND(_delegate->onQueryHostInfo(*this, info));

  send(info.encode());
}

//
// Packet:        qKillSpawnedProcess:pid
// Description:   Kill target specified
// Compatibility: LLDB
//
void Session::Handle_qKillSpawnedProcess(ProtocolInterpreter::Handler const &,
                                         std::string const &args) {
  StopInfo stop;
  ProcessId pid = std::strtoul(args.c_str(), nullptr, 10);
  sendError(_delegate->onTerminate(*this, pid, stop));
}

//
// Packet:        qL starflag threadcount nextthread
// Description:   Obtain thread information from RTOS.
// Compatibility: GDB
//
void Session::Handle_qL(ProtocolInterpreter::Handler const &,
                        std::string const &args) {
  if (args.length() < 3) {
    sendError(kErrorInvalidArgument);
    return;
  }

  ThreadId next = std::strtoul(&args[2], nullptr, 16);

  ThreadId tid;
  ErrorCode error =
      _delegate->onQueryThreadList(*this, kAnyProcessId, next, tid);
  if (error != kSuccess && error != kErrorNotFound) {
    sendError(error);
    return;
  }

  std::ostringstream ss;
  ss << "qM";
  if (error == kErrorNotFound) {
    ss << '0'; // count
    ss << '1'; // done
    ss << std::hex << std::setw(8) << std::setfill('0') << next;
    ss << std::hex << std::setw(8) << std::setfill('0') << 0;
  } else {
    ss << '1'; // count
    ss << '0'; // done
    ss << std::hex << std::setw(8) << std::setfill('0') << next;
    ss << std::hex << std::setw(8) << std::setfill('0') << tid;
  }

  send(ss.str());
}

//
// Packet:        qLaunchGDBServer[;port:port][;address:address]
// Description:   Have the remote platform launch a debugging server
// Compatibility: LLDB
//
void Session::Handle_qLaunchGDBServer(ProtocolInterpreter::Handler const &,
                                      std::string const &args) {
  std::string host;
  uint16_t port = 0;

  ParseList(args, ';', [&](std::string const &arg) {
    if (arg.compare(0, 5, "host:") == 0) {
      host = &arg[5];
    } else if (arg.compare(0, 5, "port:") == 0) {
      port = std::strtoul(&arg[5], nullptr, 10);
    }
  });

  ProcessId pid = 0;
  if (_delegate->onLaunchDebugServer(*this, host, port, pid) != kSuccess) {
    // Error is signalled setting port and pid to zero.
    port = 0;
    pid = 0;
  }

  std::ostringstream ss;
  ss << "port:" << port << ';' << "pid:" << pid << ';';
  send(ss.str());
}

//
// Packet:        qLaunchSuccess
// Description:   Returns whether the launched process has succeeded.
// Compatibility: LLDB
//
void Session::Handle_qLaunchSuccess(ProtocolInterpreter::Handler const &,
                                    std::string const &) {
  ErrorCode error = _delegate->onQueryLaunchSuccess(*this, kAnyProcessId);
  if (error != kSuccess) {
    //
    // LLDB expects E<string> and not E<code>
    //
    std::ostringstream ss;
    ss << 'E' << GetErrorCodeString(error);
    send(ss.str());
    return;
  }

  sendOK();
}

//
// Packet:        qMemoryRegionInfo:addr
// Description:   Returns information about the memory region
//                specified by address.
// Compatibility: LLDB
//
void Session::Handle_qMemoryRegionInfo(ProtocolInterpreter::Handler const &,
                                       std::string const &args) {
  // This is just a query packet
  if (args.empty()) {
    sendOK();
    return;
  }

  uint64_t address = strtoull(args.c_str(), nullptr, 16);
  MemoryRegionInfo info;
  CHK_SEND(_delegate->onQueryMemoryRegionInfo(*this, address, info));

  send(info.encode());
}

//
// Packet:        qModuleInfo:<module_path>;<arch triple>
// Description:   Get information for a module by given module path and
//                architecture.
// Compatibility: LLDB
//
void Session::Handle_qModuleInfo(ProtocolInterpreter::Handler const &,
                                 std::string const &args) {
  size_t semicolon = args.find(';');
  std::string path(HexToString(args.substr(0, semicolon)));
  std::string triple(HexToString(args.substr(semicolon + 1)));
  SharedLibraryInfo info;

  CHK_SEND(_delegate->onQuerySharedLibraryInfo(*this, path, triple, info));

  // FIXME: send the actual response.
  sendOK();
}

//
// Packet:        qOffsets
// Description:   Get section offsets that the target used when relocating
//                the downloaded image.
// Compatibility: GDB
//
void Session::Handle_qOffsets(ProtocolInterpreter::Handler const &,
                              std::string const &) {
  Address text;
  Address data;
  bool isSegment = false;

  CHK_SEND(_delegate->onQuerySectionOffsets(*this, text, data, isSegment));

  std::ostringstream ss;
  if (text.valid()) {
    ss << "Text";
    if (isSegment) {
      ss << "Seg";
    }
    ss << '=' << formatAddress(text, kEndianBig);
  }
  if (data.valid()) {
    if (text.valid()) {
      ss << ';';
    }
    ss << "Data";
    if (isSegment) {
      ss << "Seg";
    }
    ss << '=' << formatAddress(data, kEndianBig);
  }
  send(ss.str());
}

//
// Packet:        qP mode thread-id
// Description:   Returns information on thread-id.
// Compatibility: GDB
//
void Session::Handle_qP(ProtocolInterpreter::Handler const &,
                        std::string const &args) {
  if (args.length() < 8) {
    sendError(kErrorInvalidArgument);
    return;
  }

  ProcessThreadId ptid;
  if (!ptid.parse(&args[8], _compatMode)) {
    sendError(kErrorInvalidArgument);
    return;
  }

  // TODO check remote.c:remote_unpack_thread_info_response()
  std::string desc;
  CHK_SEND(_delegate->onQueryThreadInfo(*this, ptid, 0, &desc));

  send(ToHex(desc));
}

//
// Packet:        qPlatform_chmod:mode,path
// Description:   Change permissions of a file on the remote.
// Compatibility: LLDB
//
void Session::Handle_qPlatform_chmod(ProtocolInterpreter::Handler const &,
                                     std::string const &args) {
  char *eptr;
  uint32_t mode = std::strtoul(args.c_str(), &eptr, 16);
  if (*eptr++ != ',') {
    sendError(kErrorInvalidArgument);
    return;
  }

  ErrorCode error =
      _delegate->onFileSetPermissions(*this, HexToString(eptr), mode);
  if (error != kSuccess) {
    sendError(error);
    return;
  }

  // Send F + <return code>, which is always 0 on success
  send("F0");
}

//
// Packet:        qPlatform_mkdir:mode,path
// Description:   Creates a directory on the remote target.
// Compatibility: LLDB
//
void Session::Handle_qPlatform_mkdir(ProtocolInterpreter::Handler const &,
                                     std::string const &args) {
  char *eptr;
  uint32_t mode = std::strtoul(args.c_str(), &eptr, 16);
  if (*eptr++ != ',') {
    sendError(kErrorInvalidArgument);
    return;
  }

  CHK_SEND(_delegate->onFileCreateDirectory(*this, HexToString(eptr), mode));

  // Send F + <return code>, which is always 0 on success
  send("F0");
}

//
// Packet:        qPlatform_shell
// Description:   Execute a command and redirect output to the debugger
//                using F reply packets.
// Compatibility: LLDB
//
void Session::Handle_qPlatform_shell(ProtocolInterpreter::Handler const &,
                                     std::string const &args) {
  size_t comma = args.find(',');
  std::string workingDir;
  std::string command(HexToString(args.substr(0, comma)));

  char *eptr;
  uint32_t timeout = std::strtoul(&args[comma + 1], &eptr, 16);
  if (*eptr++ == ',') {
    workingDir = HexToString(eptr);
  }

  ProgramResult result;
  ErrorCode error =
      _delegate->onExecuteProgram(*this, command, timeout, workingDir, result);
  if (error != kSuccess) {
    if (error >= kErrorUnknown) {
      sendError(error);
      return;
    }

    // F,-1 in case of failure
    std::ostringstream ss;
    ss << 'F' << ',' << std::hex << error;
    send(ss.str());
  } else {
    send(result.encode(), true);
  }
}

//
// Packet:        qProcessInfo
// Description:   Get information about the process we are currently
//                debugging.
// Compatibility: LLDB
//
void Session::Handle_qProcessInfo(ProtocolInterpreter::Handler const &,
                                  std::string const &) {
  ProcessInfo info;
  CHK_SEND(_delegate->onQueryProcessInfo(*this, info));

  send(info.encode(_compatMode));
}

//
// Packet:        qProcessInfoPID:pid
// Description:   Return an indication of whether the remote server attached
//                to an existing process or created a new process.
// Compatibility: LLDB
//
void Session::Handle_qProcessInfoPID(ProtocolInterpreter::Handler const &,
                                     std::string const &args) {
  ProcessId pid = std::strtoul(args.c_str(), nullptr, 10);

  ProcessInfo info;
  CHK_SEND(_delegate->onQueryProcessInfo(*this, pid, info));

  send(info.encode(_compatMode, true));
}

//
// Packet:        qRcmd,command
// Description:   Command is passed to the local interpreter for execution
// Compatibility: GDB
//
void Session::Handle_qRcmd(ProtocolInterpreter::Handler const &,
                           std::string const &args) {
  std::string cmd = HexToString(args);

  // Special-case the exit command, since the handler will not
  // return from an exit, and we need to send an OK packet.
  if (cmd == "exit") {
    sendOK();
    _delegate->onExitServer(*this);
    DS2_UNREACHABLE();
  }

  sendError(_delegate->onExecuteCommand(*this, cmd));
}

//
// Packet:        qRegisterInfo reg
// Description:   Discover register information.
// Compatibility: LLDB
//
void Session::Handle_qRegisterInfo(ProtocolInterpreter::Handler const &,
                                   std::string const &args) {
  uint32_t regno = std::strtoul(args.c_str(), nullptr, 16);

  RegisterInfo info;
  CHK_SEND(_delegate->onQueryRegisterInfo(*this, regno, info));

  send(info.encode());
}

//
// Packet:        qSearch:memory:address;length;search-pattern
// Description:   Search in the memory interval specified the pattern.
// Compatibility: GDB
//
void Session::Handle_qSearch(ProtocolInterpreter::Handler const &,
                             std::string const &args) {
  if (args.compare(0, 7, "memory:") != 0) {
    sendError(kErrorUnsupported);
    return;
  }

  char *eptr;
  uint64_t address = strtoull(&args[7], &eptr, 16);
  if (*eptr++ != ';') {
    sendError(kErrorInvalidArgument);
    return;
  }
  uint64_t length = strtoull(eptr, &eptr, 16);
  if (*eptr++ != ';') {
    sendError(kErrorInvalidArgument);
    return;
  }

  Address location;
  ErrorCode error = _delegate->onSearch(
      *this, address, std::string(HexToString(eptr), length), location);
  if (error != kSuccess && error != kErrorNotFound) {
    sendError(error);
    return;
  }

  std::ostringstream ss;
  if (error == kErrorNotFound) {
    ss << '0';
  } else {
    ss << '1' << ',' << formatAddress(address, kEndianBig);
  }
  send(ss.str());
}

//
// Packet:        qShlibInfoAddr
// Description:   Get an address where the dynamic linker stores
//                information about where shared libraries are loaded.
// Compatibility: LLDB
//
void Session::Handle_qShlibInfoAddr(ProtocolInterpreter::Handler const &,
                                    std::string const &) {
  Address address;
  ErrorCode error =
      _delegate->onQuerySharedLibrariesInfoAddress(*this, address);
  if (error == kSuccess && !address.valid()) {
    error = kErrorInvalidArgument;
  }
  if (error != kSuccess) {
    sendError(error);
    return;
  }

  send(formatAddress(address, kEndianBig));
}

//
// Packet:        qSpeedTest:response_size:n;data:data;
// Description:   Tests speed of the link.
// Compatibility: LLDB
//
void Session::Handle_qSpeedTest(ProtocolInterpreter::Handler const &,
                                std::string const &args) {
  size_t response_size_start = 0;
  size_t response_size_end = args.find(':', response_size_start);
  if (response_size_end == std::string::npos) {
    sendError(kErrorInvalidArgument);
    return;
  }
  size_t response_size_value_start = response_size_end + 1;
  size_t response_size_value_end = args.find(';', response_size_value_start);
  if (response_size_value_end == std::string::npos) {
    sendError(kErrorInvalidArgument);
    return;
  }

  // The `data` field just serves as a placeholder for the debugger to send the
  // debugserver packets of arbitrary size.

  uint64_t response_size =
      strtoull(args.substr(response_size_value_start,
                           response_size_value_end - response_size_value_start)
                   .c_str(),
               nullptr, 10);

  if (response_size == 0) {
    sendOK();
  } else {
    std::string result = "data:";
    const size_t total_target_size = result.length() + response_size;
    result.reserve(total_target_size);
    while (result.length() < total_target_size) {
      static const char filler_data[] = "1234567890qwertyuiopasdfghjklzxcvbnm";
      static const size_t filler_data_size = sizeof(filler_data) - 1;
      result.append(filler_data, std::min(filler_data_size,
                                          total_target_size - result.length()));
    }
    send(result);
  }
}

//
// Packet:        qStepPacketSupported
// Description:   Returns whether the "s" packet is supported.
// Compatibility: GDB, LLDB
//
void Session::Handle_qStepPacketSupported(ProtocolInterpreter::Handler const &,
                                          std::string const &) {
  // We do support single stepping.
  sendOK();
}

//
// Packet:        qSupported[:gdbfeature[;gdbfeature]...]
// Description:   Tell the remote stub about features supported by
//                debugger.
// Compatibility: GDB
//
void Session::Handle_qSupported(ProtocolInterpreter::Handler const &,
                                std::string const &args) {
  Feature::Collection remoteFeatures;
  Feature::Collection localFeatures;

  //
  // Parse remote features
  // feature+;feature-;feature=value;feature?
  //
  // At the session level we're interested only in
  // multiprocess feature.
  //
  ParseList(args, ';', [&](std::string const &arg) {
    remoteFeatures.push_back(arg);

    Feature const &feature = remoteFeatures.back();
    if (feature.name == "multiprocess" && feature.flag == Feature::kSupported) {
      DS2LOG(Debug, "entering GDB multiprocess compatibility mode");
      _compatMode = kCompatibilityModeGDBMultiprocess;
    }
  });

  CHK_SEND(_delegate->onQuerySupported(*this, remoteFeatures, localFeatures));

  //
  // Build the local features response.
  //
  std::string result;
  for (auto feature : localFeatures) {
    if (feature.name.empty())
      continue;

    if (!result.empty()) {
      result += ";";
    }

    result += feature.name;
    if (!feature.value.empty()) {
      result += "=" + feature.value;
    } else
      switch (feature.flag) {
      case Feature::kSupported:
        result += "+";
        break;
      case Feature::kNotSupported:
        result += "-";
        break;
      case Feature::kQuerySupported:
        result += "?";
        break;
      }
  }

  send(result);
}

//
// Packet:        qSupportsDetachAndStayStopped
// Description:   Returns whether the supports detach and stay stopped
//                is supported.
// Compatibility: LLDB
//
void Session::Handle_qSupportsDetachAndStayStopped(
    ProtocolInterpreter::Handler const &, std::string const &) {
  // We may support this.
  sendOK();
}

//
// Packet:        qSymbol:sym_value:sym_name
// Description:   Set the value of sym_name to sym_value
//                If both sym_value and sym_name are empty then the debugger
//                is notifying the target is prepared to serve symbol lookup
//                requests.
// Compatibility: GDB
//
// Notes:
// next - a symbol name to lookup, an empty string stops querying.
//
void Session::Handle_qSymbol(ProtocolInterpreter::Handler const &,
                             std::string const &args) {
  size_t name_begin = args.find(':');
  if (name_begin == std::string::npos) {
    sendError(kErrorInvalidArgument); // Uhoh
    return;
  }

  std::string name, value;

  value = HexToString(args.substr(0, name_begin));
  if (++name_begin < args.size()) {
    name = HexToString(args.substr(name_begin));
  }

  // This is a query packet.
  if (value.empty() && name.empty()) {
    sendOK();
    return;
  }

  std::string next;
  CHK_SEND(_delegate->onQuerySymbol(*this, name, value, next));

  if (next.empty()) {
    sendOK();
  } else {
    send("qSymbol:" + ToHex(next));
  }
}

//
// Packet:        qThreadStopInfo thread-id
// Description:   Get information about why the thread specified is stopped.
// Compatibility: LLDB
//
void Session::Handle_qThreadStopInfo(ProtocolInterpreter::Handler const &,
                                     std::string const &args) {
  ProcessThreadId ptid;
  ptid.tid = strtoull(args.c_str(), nullptr, 16);

  StopInfo stop;
  CHK_SEND(_delegate->onQueryThreadStopInfo(*this, ptid, stop));

  if (_compatMode != kCompatibilityModeLLDB) {
    //
    // Update the 'c' and 'g' ptids.
    //
    _ptids['c'] = _ptids['g'] = stop.ptid;
  }

  JSArray threadsStopInfo;
  CHK_SEND(_delegate->createThreadsStopInfo(*this, threadsStopInfo));

  send(stop.encodeWithAllThreads(_compatMode, threadsStopInfo));
}

//
// Packet:        qThreadExtraInfo,thread-id
// Description:   Get printable information about the thread state.
// Compatibility: GDB, LLDB
//
void Session::Handle_qThreadExtraInfo(ProtocolInterpreter::Handler const &,
                                      std::string const &args) {
  if (args.empty()) {
    sendError(kErrorInvalidArgument);
    return;
  }

  ProcessThreadId ptid;
  if (!ptid.parse(args, _compatMode)) {
    sendError(kErrorInvalidArgument);
    return;
  }

  std::string desc;
  CHK_SEND(_delegate->onQueryThreadInfo(*this, ptid, 0, &desc));

  send(ToHex(desc));
}

//
// Packet:        qTStatus
// Description:   Query tracepoint status.
// Compatibility: GDB
//
void Session::Handle_qTStatus(ProtocolInterpreter::Handler const &,
                              std::string const &args) {
  //
  // Tracepoints are not supported right now.
  //
  sendError(kErrorUnsupported);
}

//
// Packet:        qUserName:user-id
// Description:   Query the user-id name
// Compatibility: LLDB
//
void Session::Handle_qUserName(ProtocolInterpreter::Handler const &,
                               std::string const &args) {
  std::string name;
  UserId uid = UNPACK_ID(args.c_str());
  CHK_SEND(_delegate->onQueryUserName(*this, uid, name));

  send(ToHex(name));
}

//
// Packet:        qVAttachOrWaitSupported
// Description:   Query if the 'vAttachOrWait' packet is supported
// Compatibility: LLDB
//
void Session::Handle_qVAttachOrWaitSupported(
    ProtocolInterpreter::Handler const &, std::string const &) {
  // We support it.
  sendOK();
}

//
// Packet:        qWatchpointSupportInfo
// Description:   Return the number of supported hardware watchpoints.
// Compatibility: LLDB
//
void Session::Handle_qWatchpointSupportInfo(
    ProtocolInterpreter::Handler const &, std::string const &) {
  size_t count = 0;
  CHK_SEND(_delegate->onQueryHardwareWatchpointCount(*this, count));

  std::ostringstream ss;
  ss << "num:" << count << ";";
  send(ss.str());
}

//
// Packet:        qXfer:object:read:annex:offset,length
//                qXfer:object:write:annex:offset:data...
// Description:   Read or write specific object at offset length bytes.
// Compatibility: GDB
//
void Session::Handle_qXfer(ProtocolInterpreter::Handler const &,
                           std::string const &args) {
  size_t object_start = 0;
  size_t object_end = args.find(':', object_start);
  if (object_end == std::string::npos) {
    sendError(kErrorInvalidArgument); // Malformed request
    return;
  }
  size_t action_start = object_end + 1;
  size_t action_end = args.find(':', action_start);
  if (action_end == std::string::npos) {
    sendError(kErrorInvalidArgument); // Malformed request
    return;
  }
  size_t annex_start = action_end + 1;
  size_t annex_end = args.find(':', annex_start);
  if (annex_end == std::string::npos) {
    sendError(kErrorInvalidArgument); // Malformed request
    return;
  }
  size_t offset_start = annex_end + 1;
  size_t offset_end = std::string::npos;
  size_t length_start = std::string::npos;
  size_t length_end = std::string::npos;
  size_t data_start = std::string::npos;

  std::string object = args.substr(object_start, object_end - object_start);
  std::string action = args.substr(action_start, action_end - action_start);
  std::string annex = args.substr(annex_start, annex_end - annex_start);

  if (action == "write") {
    offset_end = args.find(':', offset_start);
    if (offset_end == std::string::npos) {
      sendError(kErrorInvalidArgument); // Malformed request
      return;
    }
    data_start = offset_end + 1;

    uint64_t offset =
        strtoull(args.substr(offset_start, offset_end - offset_start).c_str(),
                 nullptr, 16);

    size_t nwritten = 0;

    CHK_SEND(_delegate->onXferWrite(*this, object, annex, offset,
                                    args.substr(data_start), nwritten));

    //
    // Send number of written bytes on success.
    //
    std::ostringstream ss;
    ss << std::hex << nwritten;
    send(ss.str());
  } else if (action == "read") {
    offset_end = args.find(',', offset_start);
    if (offset_end == std::string::npos) {
      sendError(kErrorInvalidArgument);
      return;
    }
    length_start = offset_end + 1;
    length_end = args.length();

    uint64_t offset =
        strtoull(args.substr(offset_start, offset_end - offset_start).c_str(),
                 nullptr, 16);
    uint64_t length =
        strtoull(args.substr(length_start, length_end - length_start).c_str(),
                 nullptr, 16);
    bool last = true;
    std::string buffer;

    CHK_SEND(_delegate->onXferRead(*this, object, annex, offset, length, buffer,
                                   last));

    send((last || buffer.empty() ? "l" : "m") + buffer);
  } else {
    sendError(kErrorInvalidArgument);
  }
}

//
// Packet:
// qfProcessInfo[:[name:name][;name_match:[equals|starts_with|ends_with|contains|regex]][;pid:n][;parent_pid:n][;uid:n][;gid:n][;euid:n][;egid:n][;all_users:[true|false]][;triple:triple]]
// Description:   Query first process in the system.
// Compatibility: LLDB
//
void Session::Handle_qfProcessInfo(ProtocolInterpreter::Handler const &,
                                   std::string const &args) {
  ProcessInfoMatch match;

  ParseList(args, ';', [&](std::string const &arg) {
    std::string key, value;
    size_t colon = args.find(':');
    if (colon == std::string::npos)
      return;

    key = args.substr(0, colon);
    value = args.substr(colon + 1);

    if (key == "name") {
      match.name = value;
    } else if (key == "name_match") {
      match.nameMatch = value;
    } else if (key == "pid") {
      match.pid = std::strtoul(value.c_str(), nullptr, 10);
    } else if (key == "uid") {
      match.realUid = UNPACK_ID(value.c_str());
    } else if (key == "gid") {
      match.realGid = UNPACK_ID(value.c_str());
#if !defined(OS_WIN32)
    } else if (key == "parent_pid") {
      match.parentPid = std::strtoul(value.c_str(), nullptr, 10);
    } else if (key == "euid") {
      match.effectiveUid = std::strtoul(value.c_str(), nullptr, 10);
    } else if (key == "egid") {
      match.effectiveGid = std::strtoul(value.c_str(), nullptr, 10);
#endif
    } else if (key == "all_users") {
      match.allUsers =
          (std::strtoul(value.c_str(), nullptr, 10) != 0 || value == "true");
    } else if (key == "triple") {
      match.triple = value;
    }
    match.keys.push_back(key);
  });

  ProcessInfo info;
  CHK_SEND(_delegate->onQueryProcessList(*this, match, true, info));

  send(info.encode(_compatMode, true));
}

//
// Packet:        qsProcessInfo
// Description:   Query next process in the system.
// Compatibility: LLDB
//
void Session::Handle_qsProcessInfo(ProtocolInterpreter::Handler const &,
                                   std::string const &) {
  ProcessInfo info;
  CHK_SEND(
      _delegate->onQueryProcessList(*this, ProcessInfoMatch(), false, info));

  send(info.encode(_compatMode, true));
}

//
// Packet:        qfThreadInfo
// Description:   Query first thread in the process
// Compatibility: GDB, LLDB
//
void Session::Handle_qfThreadInfo(ProtocolInterpreter::Handler const &,
                                  std::string const &) {
  ThreadId tid;
  ErrorCode error =
      _delegate->onQueryThreadList(*this, kAnyProcessId, kAllThreadId, tid);
  if (error != kSuccess && error != kErrorNotFound) {
    sendError(error);
    return;
  }

  if (error == kErrorNotFound) {
    send("l");
  } else {
    std::ostringstream ss;
    ss << "m" << getPacketSeparator() << std::hex << tid;
    send(ss.str());
  }
}

//
// Packet:        qsThreadInfo
// Description:   Query next thread in the process.
// Compatibility: GDB, LLDB
//
void Session::Handle_qsThreadInfo(ProtocolInterpreter::Handler const &,
                                  std::string const &) {
  ThreadId tid;
  ErrorCode error =
      _delegate->onQueryThreadList(*this, kAnyProcessId, kAnyThreadId, tid);
  if (error != kSuccess && error != kErrorNotFound) {
    sendError(error);
    return;
  }

  if (error == kErrorNotFound) {
    send("l");
  } else {
    std::ostringstream ss;
    ss << "m" << getPacketSeparator() << std::hex << tid;
    send(ss.str());
  }
}

//
// Packet:        R XX
// Description:   Restarts the process.
// Compatibility: GDB, LLDB
//
void Session::Handle_R(ProtocolInterpreter::Handler const &,
                       std::string const &) {
  sendError(_delegate->onRestart(*this, kAnyProcessId));
}

//
// Packet:        r
// Description:   Resets the system
// Compatibility: GDB
//
void Session::Handle_r(ProtocolInterpreter::Handler const &,
                       std::string const &) {
  _delegate->onReset(*this);
}

//
// Packet:        S sig[;addr][;thread:tid]
// Description:   Single step at the address specified if any, using
//                the specified signal.
// Compatibility: GDB, LLDB
//
void Session::Handle_S(ProtocolInterpreter::Handler const &,
                       std::string const &args) {
  ProcessThreadId ptid;
  int signal;
  Address address;
  char *bptr;
  char *eptr;

  signal = std::strtol(args.c_str(), &bptr, 16);

  if (*bptr == ';' && parseAddress(address, bptr + 1, &eptr, kEndianNative)) {
    bptr = eptr;
  }

  if (_compatMode == kCompatibilityModeLLDB) {
    //
    // LLDB will send ;thread:tid when sending this command; pid
    // is deduced from current process.
    //
    if (*bptr++ == ';') {
      if (!ptid.parse(bptr, kCompatibilityModeLLDBThread)) {
        sendError(kErrorInvalidArgument);
        return;
      }
    }
  } else {
    //
    // Use what previously has been set with H packet.
    //
    ptid = _ptids['c'];
  }

  ThreadResumeAction action;
  action.action = kResumeActionSingleStepWithSignal;
  action.ptid = ptid;
  action.signal = signal;
  action.address = address;

  ThreadResumeAction::Collection actions;
  actions.push_back(action);

  StopInfo stop;
  CHK_SEND(_delegate->onResume(*this, actions, stop));

  send(stop.encode(_compatMode, _threadsInStopReply));

  if (_compatMode != kCompatibilityModeLLDB) {
    //
    // Update the 'c' and 'g' ptids.
    //
    _ptids['c'] = _ptids['g'] = stop.ptid;
  }
}

//
// Packet:        s [addr][;thread:tid]
// Description:   Single step at the address specified if any.
// Compatibility: GDB, LLDB
//
void Session::Handle_s(ProtocolInterpreter::Handler const &,
                       std::string const &args) {
  ProcessThreadId ptid;
  Address address;
  char *eptr;

  parseAddress(address, args.c_str(), &eptr, kEndianNative);

  if (_compatMode == kCompatibilityModeLLDB) {
    //
    // LLDB will send ;thread:tid when sending this command; pid
    // is deduced from current process.
    //
    if (*eptr++ == ';') {
      if (!ptid.parse(eptr, kCompatibilityModeLLDBThread)) {
        sendError(kErrorInvalidArgument);
        return;
      }
    }
  } else {
    //
    // Use what previously has been set with H packet.
    //
    ptid = _ptids['c'];
  }

  ThreadResumeAction action;
  action.action = kResumeActionSingleStep;
  action.ptid = ptid;
  action.address = address;

  ThreadResumeAction::Collection actions;
  actions.push_back(action);

  StopInfo stop;
  CHK_SEND(_delegate->onResume(*this, actions, stop));

  send(stop.encode(_compatMode, _threadsInStopReply));

  if (_compatMode != kCompatibilityModeLLDB) {
    //
    // Update the 'c' and 'g' ptids.
    //
    _ptids['c'] = _ptids['g'] = stop.ptid;
  }
}

//
// Packet:        T thread-id
// Description:   Query if thread specified is alive.
// Compatibility: GDB
//
void Session::Handle_T(ProtocolInterpreter::Handler const &,
                       std::string const &args) {
  ProcessThreadId ptid;
  if (!ptid.parse(args, _compatMode)) {
    sendError(kErrorInvalidArgument);
    return;
  }

  sendError(_delegate->onThreadIsAlive(*this, ptid));
}

//
// Packet:        t addr:PP,MM
// Description:   Search backwards starting at address addr for a match with
//                pattern PP and mask MM. PP and MM are 4 bytes. addr must
//                be at least 3 hex digits.
// Compatibility: GDB
//
void Session::Handle_t(ProtocolInterpreter::Handler const &,
                       std::string const &args) {
  char *eptr;
  uint64_t address = strtoull(args.c_str(), &eptr, 16);
  if (*eptr++ != ':') {
    sendError(kErrorInvalidArgument);
    return;
  }
  uint32_t pattern = std::strtoul(eptr, &eptr, 16);
  if (*eptr++ != ',') {
    sendError(kErrorInvalidArgument);
    return;
  }
  uint32_t mask = std::strtoul(eptr, &eptr, 16);

  Address location;
  CHK_SEND(
      _delegate->onSearchBackward(*this, address, pattern, mask, location));

  send(formatAddress(location, kEndianBig));
}

//
// Packet:        vAttach;pid
// Description:   Attach to an existing process.
// Compatibility: GDB, LLDB
//
void Session::Handle_vAttach(ProtocolInterpreter::Handler const &,
                             std::string const &args) {
  StopInfo stop;
  ProcessId pid = std::strtoul(args.c_str(), nullptr, 16);
  CHK_SEND(_delegate->onAttach(*this, pid, kAttachNow, stop));

  send(stop.encode(_compatMode, _threadsInStopReply));

  if (_compatMode != kCompatibilityModeLLDB) {
    //
    // Update the 'c' and 'g' ptids.
    //
    _ptids['c'] = _ptids['g'] = stop.ptid;
  }
}

//
// Packet:        vAttachName;process-name
// Description:   Attach to a named existing process.
// Compatibility: LLDB
//
void Session::Handle_vAttachName(ProtocolInterpreter::Handler const &,
                                 std::string const &args) {
  StopInfo stop;
  CHK_SEND(_delegate->onAttach(*this, HexToString(args), kAttachNow, stop));

  send(stop.encode(_compatMode, _threadsInStopReply));

  if (_compatMode != kCompatibilityModeLLDB) {
    //
    // Update the 'c' and 'g' ptids.
    //
    _ptids['c'] = _ptids['g'] = stop.ptid;
  }
}

//
// Packet:        vAttachOrWait;process-name
// Description:   Attach to the process or if it doesn't exist, wait
//                for the process to start up then attach to it.
// Compatibility: LLDB
//
void Session::Handle_vAttachOrWait(ProtocolInterpreter::Handler const &,
                                   std::string const &args) {
  StopInfo stop;
  CHK_SEND(_delegate->onAttach(*this, HexToString(args), kAttachOrWait, stop));

  send(stop.encode(_compatMode, _threadsInStopReply));

  if (_compatMode != kCompatibilityModeLLDB) {
    //
    // Update the 'c' and 'g' ptids.
    //
    _ptids['c'] = _ptids['g'] = stop.ptid;
  }
}

//
// Packet:        vAttachWait;process-name
// Description:   Wait for a process to start up then attach to it.
// Compatibility: LLDB
//
void Session::Handle_vAttachWait(ProtocolInterpreter::Handler const &,
                                 std::string const &args) {
  StopInfo stop;
  CHK_SEND(_delegate->onAttach(*this, HexToString(args), kAttachAndWait, stop));

  send(stop.encode(_compatMode, _threadsInStopReply));

  if (_compatMode != kCompatibilityModeLLDB) {
    //
    // Update the 'c' and 'g' ptids.
    //
    _ptids['c'] = _ptids['g'] = stop.ptid;
  }
}

//
// Packet:        vCont?
// Description:   Request a list of actions supported by vCont
// Compatibility: GDB, LLDB
//
void Session::Handle_vContQuestionMark(ProtocolInterpreter::Handler const &,
                                       std::string const &) {
  // We support all the actions!
  send("vCont;t;s;S;c;C;");
}

//
// Packet:        vCont[;action[:thread-id]]...
// Description:   Resume the inferior
// Compatibility: GDB, LLDB
//
void Session::Handle_vCont(ProtocolInterpreter::Handler const &,
                           std::string const &args) {
  ThreadResumeAction::Collection actions;

  if (!ParseList(args, ';', [&](std::string const &arg) {
        if (arg.empty())
          return;

        ThreadResumeAction action;
        char *eptr = const_cast<char *>(&arg[0]);
        switch (*eptr++) {
        case 'c':
          action.action = kResumeActionContinue;
          action.signal = 0;
          break;
        case 'C':
          action.action = kResumeActionContinueWithSignal;
          action.signal = std::strtoul(eptr, &eptr, 16);
          break;
        case 's':
          action.action = kResumeActionSingleStep;
          action.signal = 0;
          break;
        case 'S':
          action.action = kResumeActionSingleStepWithSignal;
          action.signal = std::strtoul(eptr, &eptr, 16);
          break;
        case 't':
          action.action = kResumeActionStop;
          action.signal = 0;
          break;
        default:
          sendError(kErrorInvalidArgument); // Not supported
          return;
        }
        if (*eptr++ == ':') {
          if (!action.ptid.parse(eptr, _compatMode)) {
            sendError(kErrorInvalidArgument); // Not supported
            return;
          }
        }

        actions.push_back(action);
      })) {
    sendError(kErrorInvalidArgument);
    return;
  }

  StopInfo stop;
  CHK_SEND(_delegate->onResume(*this, actions, stop));

  send(stop.encode(_compatMode, _threadsInStopReply));

  if (_compatMode != kCompatibilityModeLLDB) {
    //
    // Update the 'c' and 'g' ptids.
    //
    _ptids['c'] = _ptids['g'] = stop.ptid;
  }
}

//
// Packet:        vFile:operation:parameter...
// Description:   Perform a file operation on target.
// Compatibility: GDB, LLDB
//
void Session::Handle_vFile(ProtocolInterpreter::Handler const &,
                           std::string const &args) {
  size_t op_start = 0;
  size_t op_end = args.find(':', op_start);
  if (op_end == std::string::npos) {
    sendError(kErrorInvalidArgument); // Malformed request
    return;
  }

  std::ostringstream ss;

  std::string op = args.substr(op_start, op_end);
  op_end++;

  // lldb encodes count and fd's in decimal, gdb encodes them in hex
  int base = (_compatMode == kCompatibilityModeLLDB) ? 10 : 16;
  auto baseModifier =
      (_compatMode == kCompatibilityModeLLDB) ? std::dec : std::hex;

  //
  // GDB:  vFile:open:path,flags,mode
  //       vFile:close:fd
  //       vFile:pread:fd,count,offset
  //       vFile:pwrite:fd,offset,data
  //       vFile:unlink:path
  //       vFile:readlink:path
  //
  // LLDB: vFile:exists:path
  //       vFile:size:path
  //       vFile:MD5:path
  //
  bool escaped = false;
  if (op == "open") {
    size_t comma = args.find(',', op_end);
    if (comma == std::string::npos) {
      sendError(kErrorInvalidArgument);
      return;
    }

    char *eptr;
    uint32_t flags = std::strtoul(&args[comma + 1], &eptr, 16);
    if (*eptr++ != ',') {
      sendError(kErrorInvalidArgument);
      return;
    }

    OpenFlags openFlags = ConvertOpenFlags(flags);
    if (openFlags == kOpenFlagInvalid) {
      sendError(kErrorInvalidArgument);
      return;
    }

    uint32_t mode = std::strtoul(eptr, nullptr, 16);

    int fd;
    ErrorCode error = _delegate->onFileOpen(
        *this, HexToString(args.substr(op_end, comma - op_end)), openFlags,
        mode, fd);
    if (error != kSuccess) {
      ss << 'F' << -1 << ',' << std::hex << error;
    } else {
      ss << 'F' << baseModifier << fd;
    }
  } else if (op == "close") {
    int fd = std::strtol(&args[op_end], nullptr, base);
    ErrorCode error = _delegate->onFileClose(*this, fd);
    if (error != kSuccess) {
      ss << 'F' << -1 << ',' << std::hex << error;
    } else {
      ss << 'F' << 0;
    }
  } else if (op == "pread") {
    char *eptr;
    int fd = std::strtol(&args[op_end], &eptr, base);
    if (*eptr++ != ',') {
      sendError(kErrorInvalidArgument);
      return;
    }
    uint64_t count = strtoull(eptr, &eptr, base);
    if (*eptr++ != ',') {
      sendError(kErrorInvalidArgument);
      return;
    }
    uint64_t offset = strtoull(eptr, &eptr, base);

    ByteVector buffer;
    ErrorCode error = _delegate->onFileRead(*this, fd, count, offset, buffer);
    if (error != kSuccess) {
      ss << 'F' << -1 << ',' << std::hex << error;
    } else {
      ss << 'F' << baseModifier << count << ';' << Escape(buffer);
      escaped = true;
    }
  } else if (op == "pwrite") {
    char *eptr;
    int fd = std::strtol(&args[op_end], &eptr, base);
    if (*eptr++ != ',') {
      sendError(kErrorInvalidArgument);
      return;
    }
    uint64_t offset = strtoull(eptr, &eptr, base);
    if (*eptr++ != ',') {
      sendError(kErrorInvalidArgument);
      return;
    }

    auto bytePtr = reinterpret_cast<uint8_t *>(eptr);
    uint64_t length = args.length() - (eptr - args.c_str());
    if (length == 0) {
      sendError(kErrorInvalidArgument);
      return;
    }
    ErrorCode error = _delegate->onFileWrite(
        *this, fd, offset, ByteVector(bytePtr, bytePtr + length), length);
    if (error != kSuccess) {
      ss << 'F' << -1 << ',' << std::hex << error;
    } else {
      ss << 'F' << baseModifier << length;
    }
  } else if (op == "unlink") {
    ErrorCode error =
        _delegate->onFileRemove(*this, HexToString(&args[op_end]));
    if (error != kSuccess) {
      ss << 'F' << -1 << ',' << std::hex << error;
    } else {
      ss << 'F' << 0;
    }
  } else if (op == "readlink") {
    std::string resolved;
    ErrorCode error =
        _delegate->onFileReadLink(*this, HexToString(&args[op_end]), resolved);
    if (error != kSuccess) {
      ss << 'F' << -1 << ',' << std::hex << error;
    } else {
      ss << 'F' << 0 << ';' << ToHex(resolved);
    }
  } else if (op == "exists") {
    ErrorCode error =
        _delegate->onFileExists(*this, HexToString(&args[op_end]));
    // F,<bool>
    ss << 'F' << ',' << (error != kSuccess ? 0 : 1);
  } else if (op == "MD5") {
    uint8_t digest[16];
    ErrorCode error =
        _delegate->onFileComputeMD5(*this, HexToString(&args[op_end]), digest);
    ss << 'F' << ',';
    // F,<value> or F,x if not found
    if (error != kSuccess) {
      ss << 'x';
    } else {
      for (unsigned char n : digest) {
        ss << std::hex << std::setw(2) << std::setfill('0') << n;
      }
    }
  } else if (op == "size") {
    uint64_t size;
    ErrorCode error =
        _delegate->onFileGetSize(*this, HexToString(&args[op_end]), size);
    // Fsize or Exx if error.
    if (error != kSuccess) {
      ss << 'E' << std::hex << error;
    } else {
      ss << 'F' << baseModifier << size;
    }
  } else {
    sendError(kErrorUnsupported);
    return;
  }

  send(ss.str(), escaped);
}

//
// Packet:        vFlashDone
// Description:   Indicates that the write operation is done.
// Compatibility: GDB
//
void Session::Handle_vFlashDone(ProtocolInterpreter::Handler const &,
                                std::string const &) {
  sendError(_delegate->onFlashDone(*this));
}

//
// Packet:        vFlashErase:addr,length
// Description:   Erases flash memory at the address specified.
// Compatibility: GDB
//
void Session::Handle_vFlashErase(ProtocolInterpreter::Handler const &,
                                 std::string const &args) {
  uint64_t address;
  uint64_t length;
  char *eptr;

  address = strtoull(args.c_str(), &eptr, 16);
  if (*eptr++ != ',') {
    sendError(kErrorInvalidArgument); // Uhoh
    return;
  }
  length = strtoull(eptr, nullptr, 16);

  sendError(_delegate->onFlashErase(*this, address, length));
}

//
// Packet:        vFlashWrite:addr,length:XX...
// Description:   Writes flash memory at the address specified.
// Compatibility: GDB
//
void Session::Handle_vFlashWrite(ProtocolInterpreter::Handler const &,
                                 std::string const &args) {
  char *eptr;
  uint64_t address;
  uint64_t length;

  address = strtoull(args.c_str(), &eptr, 16);
  if (*eptr++ != ',') {
    sendError(kErrorInvalidArgument);
    return;
  }
  length = strtoull(eptr, &eptr, 16);
  if (*eptr++ != ':') {
    sendError(kErrorInvalidArgument);
    return;
  }

  auto bytePtr = reinterpret_cast<uint8_t *>(eptr);
  ErrorCode error = _delegate->onFlashWrite(
      *this, address, ByteVector(bytePtr, bytePtr + length));
  if (error == kErrorInvalidAddress) {
    send("E.memtype");
  } else {
    sendError(error);
  }
}

//
// Packet:        vKill[;pid]
// Description:   Kill target specified
// Compatibility: GDB, LLDB
//
void Session::Handle_vKill(ProtocolInterpreter::Handler const &,
                           std::string const &args) {
  ProcessId pid = std::strtoul(args.c_str(), nullptr, 16);

  StopInfo stop;
  CHK_SEND(_delegate->onTerminate(*this, pid, stop));

  send(stop.encode(_compatMode, _threadsInStopReply));

  if (_compatMode != kCompatibilityModeLLDB) {
    //
    // Update the 'c' and 'g' ptids.
    //
    _ptids['c'] = _ptids['g'] = stop.ptid;
  }
}

//
// Packet:        vRun;filename[;argument[;...]]
// Description:   Run an executable and attach to it.
// Compatibility: GDB
//
void Session::Handle_vRun(ProtocolInterpreter::Handler const &,
                          std::string const &args) {
  std::string filename;
  StringCollection arguments;
  size_t index = 0;

  ParseList(args, ';', [&](std::string const &arg) {
    if (index == 0) {
      filename = HexToString(arg);
    } else {
      arguments.push_back(HexToString(arg));
    }
  });

  StopInfo stop;
  CHK_SEND(_delegate->onRunAttach(*this, filename, arguments, stop));

  send(stop.encode(_compatMode, _threadsInStopReply));

  if (_compatMode != kCompatibilityModeLLDB) {
    //
    // Update the 'c' and 'g' ptids.
    //
    _ptids['c'] = _ptids['g'] = stop.ptid;
  }
}

//
// Packet:        vStopped
// Description:   Reply the thread stop code.
// Compatibility: GDB
//
void Session::Handle_vStopped(ProtocolInterpreter::Handler const &,
                              std::string const &) {
  StopInfo stop;
  ErrorCode error =
      _delegate->onQueryThreadStopInfo(*this, ProcessThreadId(), stop);
  if (error != kSuccess && error != kErrorNotFound) {
    sendError(error);
    return;
  }

  if (error == kSuccess) {
    send(stop.encode(_compatMode, _threadsInStopReply));

    if (_compatMode != kCompatibilityModeLLDB) {
      //
      // Update the 'c' and 'g' ptids.
      //
      _ptids['c'] = _ptids['g'] = stop.ptid;
    }
  } else {
    sendOK();
  }
}

//
// Packet:        X addr,length:XX...
// Description:   Write to target memory, data is binary.
// Compatibility: GDB, LLDB
//
void Session::Handle_X(ProtocolInterpreter::Handler const &,
                       std::string const &args) {
  char *eptr;
  uint64_t address;
  uint64_t length;

  address = strtoull(args.c_str(), &eptr, 16);
  if (*eptr++ != ',') {
    sendError(kErrorInvalidArgument);
    return;
  }
  length = strtoull(eptr, &eptr, 16);
  if (*eptr++ != ':') {
    sendError(kErrorInvalidArgument);
    return;
  }

  size_t nwritten = 0;
  auto bytePtr = reinterpret_cast<uint8_t *>(eptr);
  CHK_SEND(_delegate->onWriteMemory(
      *this, address, ByteVector(bytePtr, bytePtr + length), nwritten));

  std::ostringstream ss;
  ss << std::hex << nwritten;
  send(ss.str());
}

//
// Packet:        x addr,length:XX...
// Description:   Read from target memory, data is binary.
// Compatibility: GDB, LLDB
//
void Session::Handle_x(ProtocolInterpreter::Handler const &,
                       std::string const &args) {
  char *eptr;
  uint64_t address;
  uint64_t length;
  ByteVector data;

  address = strtoull(args.c_str(), &eptr, 16);
  if (*eptr++ != ',') {
    sendError(kErrorInvalidArgument);
    return;
  }
  length = strtoull(eptr, &eptr, 16);

  if (address == 0 && length == 0) {
    sendOK();
    return;
  }

  CHK_SEND(_delegate->onReadMemory(*this, address, length, data));

  send(data);
}

//
// Packet:        Z type,addr,kind[;cond_list...][;cmds:[persist,]cmd_list...]
// Description:   Inserts a breakpoint or watchpoint.
// Compatibility: GDB, LLDB
//
void Session::Handle_Z(ProtocolInterpreter::Handler const &,
                       std::string const &args) {
  char *eptr;
  BreakpointType type;
  uint64_t address;
  uint32_t kind;

  type = static_cast<BreakpointType>(std::strtoul(args.c_str(), &eptr, 16));
  if (type >= kBreakpointTypeMax) {
    sendError(kErrorInvalidArgument);
    return;
  }
  if (*eptr++ != ',') {
    sendError(kErrorInvalidArgument);
    return;
  }
  address = strtoull(eptr, &eptr, 16);
  if (*eptr++ != ',') {
    sendError(kErrorInvalidArgument);
    return;
  }
  kind = std::strtoul(eptr, &eptr, 16);

  // TODO: cond_list, cmd_list.
  sendError(_delegate->onInsertBreakpoint(*this, type, address, kind,
                                          StringCollection(),
                                          StringCollection(), false));
}

//
// Packet:        z type,addr,kind
// Description:   Removes a breakpoint or watchpoint.
// Compatibility: GDB, LLDB
//
void Session::Handle_z(ProtocolInterpreter::Handler const &,
                       std::string const &args) {
  char *eptr;
  BreakpointType type;
  uint64_t address;
  uint32_t kind;

  type = static_cast<BreakpointType>(std::strtoul(args.c_str(), &eptr, 16));
  if (*eptr++ != ',') {
    sendError(kErrorInvalidArgument);
    return;
  }
  address = strtoull(eptr, &eptr, 16);
  if (*eptr++ != ',') {
    sendError(kErrorInvalidArgument);
    return;
  }
  kind = std::strtoul(eptr, &eptr, 16);

  sendError(_delegate->onRemoveBreakpoint(*this, type, address, kind));
}
} // namespace GDBRemote
} // namespace ds2
