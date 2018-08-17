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

#include "DebugServer2/GDBRemote/ProtocolInterpreter.h"
#include "DebugServer2/GDBRemote/SessionBase.h"

#include <functional>
#include <map>

namespace ds2 {
namespace GDBRemote {

class Session : public SessionBase {
protected:
  std::map<char, ProcessThreadId> _ptids;
  bool _threadsInStopReply;

public:
  Session(CompatibilityMode mode);

private:
  void Handle_ControlC(ProtocolInterpreter::Handler const &,
                       std::string const &);
  void Handle_QuestionMark(ProtocolInterpreter::Handler const &,
                           std::string const &);
  void Handle_ExclamationMark(ProtocolInterpreter::Handler const &,
                              std::string const &);

private:
  void Handle_A(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_B(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_b(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_bc(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_bs(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_C(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_c(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_D(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_d(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_G(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_g(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_H(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_I(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_i(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_k(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_jThreadsInfo(ProtocolInterpreter::Handler const &,
                           std::string const &);
  void Handle__M(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle__m(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_M(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_m(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_P(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_p(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_QAgent(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_QAllow(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_Qbtrace(ProtocolInterpreter::Handler const &,
                      std::string const &);
  void Handle_QDisableRandomization(ProtocolInterpreter::Handler const &,
                                    std::string const &);
  void Handle_QEnvironment(ProtocolInterpreter::Handler const &,
                           std::string const &);
  void Handle_QEnvironmentHexEncoded(ProtocolInterpreter::Handler const &,
                                     std::string const &);
  void Handle_QLaunchArch(ProtocolInterpreter::Handler const &,
                          std::string const &);
  void Handle_QListThreadsInStopReply(ProtocolInterpreter::Handler const &,
                                      std::string const &);
  void Handle_QNonStop(ProtocolInterpreter::Handler const &,
                       std::string const &);
  void Handle_QPassSignals(ProtocolInterpreter::Handler const &,
                           std::string const &);
  void Handle_QProgramSignals(ProtocolInterpreter::Handler const &,
                              std::string const &);
  void Handle_QSetDisableASLR(ProtocolInterpreter::Handler const &,
                              std::string const &);
  void Handle_QSetEnableAsyncProfiling(ProtocolInterpreter::Handler const &,
                                       std::string const &);
  void Handle_QSetLogging(ProtocolInterpreter::Handler const &,
                          std::string const &);
  void Handle_QSetSTDERR(ProtocolInterpreter::Handler const &,
                         std::string const &);
  void Handle_QSetSTDIN(ProtocolInterpreter::Handler const &,
                        std::string const &);
  void Handle_QSetSTDOUT(ProtocolInterpreter::Handler const &,
                         std::string const &);
  void Handle_QSetMaxPacketSize(ProtocolInterpreter::Handler const &,
                                std::string const &);
  void Handle_QSetMaxPayloadSize(ProtocolInterpreter::Handler const &,
                                 std::string const &);
  void Handle_QSetWorkingDir(ProtocolInterpreter::Handler const &,
                             std::string const &);
  void Handle_QRestoreRegisterState(ProtocolInterpreter::Handler const &,
                                    std::string const &args);
  void Handle_QSaveRegisterState(ProtocolInterpreter::Handler const &,
                                 std::string const &args);
  void Handle_QStartNoAckMode(ProtocolInterpreter::Handler const &,
                              std::string const &);
  void Handle_QSyncThreadState(ProtocolInterpreter::Handler const &,
                               std::string const &);
  void Handle_QThreadSuffixSupported(ProtocolInterpreter::Handler const &,
                                     std::string const &);
  void Handle_qAttached(ProtocolInterpreter::Handler const &,
                        std::string const &);
  void Handle_qC(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_qCRC(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_qEcho(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_qFileLoadAddress(ProtocolInterpreter::Handler const &,
                               std::string const &);
  void Handle_qGDBServerVersion(ProtocolInterpreter::Handler const &,
                                std::string const &);
  void Handle_qGetPid(ProtocolInterpreter::Handler const &,
                      std::string const &);
  void Handle_qGetProfileData(ProtocolInterpreter::Handler const &,
                              std::string const &);
  void Handle_qGetTLSAddr(ProtocolInterpreter::Handler const &,
                          std::string const &);
  void Handle_qGetTIBAddr(ProtocolInterpreter::Handler const &,
                          std::string const &);
  void Handle_qGetWorkingDir(ProtocolInterpreter::Handler const &,
                             std::string const &);
  void Handle_qGroupName(ProtocolInterpreter::Handler const &,
                         std::string const &);
  void Handle_qHostInfo(ProtocolInterpreter::Handler const &,
                        std::string const &);
  void Handle_qKillSpawnedProcess(ProtocolInterpreter::Handler const &,
                                  std::string const &);
  void Handle_qL(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_qLaunchGDBServer(ProtocolInterpreter::Handler const &,
                               std::string const &);
  void Handle_qLaunchSuccess(ProtocolInterpreter::Handler const &,
                             std::string const &);
  void Handle_qMemoryRegionInfo(ProtocolInterpreter::Handler const &,
                                std::string const &);
  void Handle_qModuleInfo(ProtocolInterpreter::Handler const &,
                          std::string const &);
  void Handle_qOffsets(ProtocolInterpreter::Handler const &,
                       std::string const &);
  void Handle_qP(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_qPlatform_chmod(ProtocolInterpreter::Handler const &,
                              std::string const &);
  void Handle_qPlatform_mkdir(ProtocolInterpreter::Handler const &,
                              std::string const &);
  void Handle_qPlatform_shell(ProtocolInterpreter::Handler const &,
                              std::string const &);
  void Handle_qProcessInfo(ProtocolInterpreter::Handler const &,
                           std::string const &);
  void Handle_qProcessInfoPID(ProtocolInterpreter::Handler const &,
                              std::string const &);
  void Handle_qRcmd(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_qRegisterInfo(ProtocolInterpreter::Handler const &,
                            std::string const &);
  void Handle_qSearch(ProtocolInterpreter::Handler const &,
                      std::string const &);
  void Handle_qShlibInfoAddr(ProtocolInterpreter::Handler const &,
                             std::string const &);
  void Handle_qSpeedTest(ProtocolInterpreter::Handler const &,
                         std::string const &);
  void Handle_qStepPacketSupported(ProtocolInterpreter::Handler const &,
                                   std::string const &);
  void Handle_qSupported(ProtocolInterpreter::Handler const &,
                         std::string const &);
  void
  Handle_qSupportsDetachAndStayStopped(ProtocolInterpreter::Handler const &,
                                       std::string const &);
  void Handle_qSymbol(ProtocolInterpreter::Handler const &,
                      std::string const &);
  void Handle_qThreadStopInfo(ProtocolInterpreter::Handler const &,
                              std::string const &);
  void Handle_qThreadExtraInfo(ProtocolInterpreter::Handler const &,
                               std::string const &);
  void Handle_qTStatus(ProtocolInterpreter::Handler const &,
                       std::string const &);
  void Handle_qUserName(ProtocolInterpreter::Handler const &,
                        std::string const &);
  void Handle_qVAttachOrWaitSupported(ProtocolInterpreter::Handler const &,
                                      std::string const &);
  void Handle_qWatchpointSupportInfo(ProtocolInterpreter::Handler const &,
                                     std::string const &);
  void Handle_qXfer(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_qfProcessInfo(ProtocolInterpreter::Handler const &,
                            std::string const &);
  void Handle_qsProcessInfo(ProtocolInterpreter::Handler const &,
                            std::string const &);
  void Handle_qfThreadInfo(ProtocolInterpreter::Handler const &,
                           std::string const &);
  void Handle_qsThreadInfo(ProtocolInterpreter::Handler const &,
                           std::string const &);
  void Handle_R(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_r(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_S(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_s(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_T(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_t(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_vAttach(ProtocolInterpreter::Handler const &,
                      std::string const &);
  void Handle_vAttachName(ProtocolInterpreter::Handler const &,
                          std::string const &);
  void Handle_vAttachWait(ProtocolInterpreter::Handler const &,
                          std::string const &);
  void Handle_vAttachOrWait(ProtocolInterpreter::Handler const &,
                            std::string const &);
  void Handle_vContQuestionMark(ProtocolInterpreter::Handler const &,
                                std::string const &);
  void Handle_vCont(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_vFile(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_vFlashDone(ProtocolInterpreter::Handler const &,
                         std::string const &);
  void Handle_vFlashErase(ProtocolInterpreter::Handler const &,
                          std::string const &);
  void Handle_vFlashWrite(ProtocolInterpreter::Handler const &,
                          std::string const &);
  void Handle_vKill(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_vRun(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_vStopped(ProtocolInterpreter::Handler const &,
                       std::string const &);
  void Handle_X(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_x(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_Z(ProtocolInterpreter::Handler const &, std::string const &);
  void Handle_z(ProtocolInterpreter::Handler const &, std::string const &);

private:
  static bool ParseList(std::string const &string, char separator,
                        std::function<void(std::string const &)> const &cb);

private:
  OpenFlags ConvertOpenFlags(uint32_t protocolFlags);

private:
  bool parseAddress(Address &address, const char *ptr, char **eptr,
                    Endian endianness) const;
  std::string formatAddress(Address const &address, Endian endianness) const;
};
} // namespace GDBRemote
} // namespace ds2
