//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#define __DS2_LOG_CLASS_NAME__ "DebugSession"

#include "DebugServer2/BreakpointManager.h"
#include "DebugServer2/GDBRemote/DebugSessionImpl.h"
#include "DebugServer2/GDBRemote/Session.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Support/Stringify.h"
#include "DebugServer2/Utils/HexValues.h"
#include "DebugServer2/Utils/Log.h"

#include <sstream>
#include <iomanip>

using ds2::Host::Platform;
using ds2::Support::Stringify;
using ds2::Target::Thread;

namespace ds2 {
namespace GDBRemote {

DebugSessionImpl::DebugSessionImpl(StringCollection const &args,
                                   EnvironmentBlock const &env)
    : DummySessionDelegateImpl(), _resumeSession(nullptr) {
  DS2ASSERT(args.size() >= 1);
  _resumeSessionLock.lock();
  spawnProcess(args, env);
}

DebugSessionImpl::DebugSessionImpl(int attachPid)
    : DummySessionDelegateImpl(), _resumeSession(nullptr) {
  _resumeSessionLock.lock();
  _process = ds2::Target::Process::Attach(attachPid);
  if (_process == nullptr)
    DS2LOG(Fatal, "cannot attach to pid %d", attachPid);
}

DebugSessionImpl::DebugSessionImpl()
    : DummySessionDelegateImpl(), _process(nullptr), _resumeSession(nullptr) {
  _resumeSessionLock.lock();
}

DebugSessionImpl::~DebugSessionImpl() {
  _resumeSessionLock.unlock();
  delete _process;
}

size_t DebugSessionImpl::getGPRSize() const {
  if (_process == nullptr)
    return 0;

  ProcessInfo info;
  if (_process->getInfo(info) != kSuccess)
    return 0;

  return info.pointerSize << 3;
}

ErrorCode DebugSessionImpl::onInterrupt(Session &) {
  return _process->interrupt();
}

ErrorCode
DebugSessionImpl::onQuerySupported(Session &session,
                                   Feature::Collection const &remoteFeatures,
                                   Feature::Collection &localFeatures) const {
  for (auto feature : remoteFeatures) {
    DS2LOG(Debug, "gdb feature: %s", feature.name.c_str());
  }

  // TODO PacketSize should be respected
  localFeatures.push_back(std::string("PacketSize=3fff"));
  localFeatures.push_back(std::string("ConditionalBreakpoints-"));
  localFeatures.push_back(std::string("BreakpointCommands+"));
  localFeatures.push_back(std::string("multiprocess+"));
  localFeatures.push_back(std::string("QPassSignals+"));
  localFeatures.push_back(std::string("QStartNoAckMode+"));
  localFeatures.push_back(std::string("QDisableRandomization+"));
  localFeatures.push_back(std::string("QNonStop+"));
#if defined(OS_LINUX)
  localFeatures.push_back(std::string("QProgramSignals+"));
  localFeatures.push_back(std::string("qXfer:siginfo:read+"));
  localFeatures.push_back(std::string("qXfer:siginfo:write+"));
#else
  localFeatures.push_back(std::string("QProgramSignals-"));
  localFeatures.push_back(std::string("qXfer:siginfo:read-"));
  localFeatures.push_back(std::string("qXfer:siginfo:write-"));
#endif
#if !defined(OS_WIN32)
  localFeatures.push_back(std::string("qXfer:auxv:read+"));
  localFeatures.push_back(std::string("qXfer:libraries-svr4:read+"));
#else
  localFeatures.push_back(std::string("qXfer:libraries:read+"));
#endif
  localFeatures.push_back(std::string("qXfer:features:read+"));
  localFeatures.push_back(std::string("qXfer:osdata:read+"));
  localFeatures.push_back(std::string("qXfer:threads:read+"));
  // Disable unsupported tracepoints
  localFeatures.push_back(std::string("Qbtrace:bts-"));
  localFeatures.push_back(std::string("Qbtrace:off-"));
  localFeatures.push_back(std::string("tracenz-"));
  localFeatures.push_back(std::string("ConditionalTracepoints-"));
  localFeatures.push_back(std::string("TracepointSource-"));
  localFeatures.push_back(std::string("EnableDisableTracepoints-"));

  return kSuccess;
}

ErrorCode DebugSessionImpl::onPassSignals(Session &session,
                                          std::vector<int> const &signals) {
  _process->resetSignalPass();
  for (int signo : signals) {
    DS2LOG(Debug, "passing signal %d", signo);
    _process->setSignalPass(signo, true);
  }
  return kSuccess;
}

ErrorCode DebugSessionImpl::onProgramSignals(Session &session,
                                             std::vector<int> const &signals) {
  for (int signo : signals) {
    DS2LOG(Debug, "programming signal %d", signo);
    _process->setSignalPass(signo, false);
  }
  return kSuccess;
}

ErrorCode DebugSessionImpl::onNonStopMode(Session &session, bool enable) {
  if (enable)
    return kErrorUnsupported; // TODO support non-stop mode

  return kSuccess;
}

Thread *DebugSessionImpl::findThread(ProcessThreadId const &ptid) const {
  if (_process == nullptr)
    return nullptr;

  if (ptid.validPid() && ptid.pid != _process->pid())
    return nullptr;

  Thread *thread = nullptr;
  if (!ptid.validTid()) {
    thread = _process->currentThread();
  } else {
    thread = _process->thread(ptid.tid);
  }

  return thread;
}

ErrorCode DebugSessionImpl::queryStopCode(Session &session,
                                          ProcessThreadId const &ptid,
                                          StopCode &stop) const {
  Thread *thread = findThread(ptid);
  DS2LOG(Debug, "thread %p", thread);
  if (thread == nullptr) {
#if defined(OS_WIN32)
    if (_process && !_process->isAlive()) {
      stop.event = StopCode::kSignalExit;
      stop.signal = 9;
      return kSuccess;
    }
#else
    return kErrorProcessNotFound;
#endif
  }

  bool readRegisters = true;
  StopInfo const &info = thread->stopInfo();

  Architecture::CPUState state;

  stop.ptid.pid = thread->process()->pid();
  stop.ptid.tid = thread->tid();
  stop.core = info.core;

  // This code translate a StopInfo to a StopCode that we can send back to the
  // debugger.
  switch (info.event) {
  case StopInfo::kEventNone:
    stop.event = StopCode::kSignal;
    stop.reason = StopInfo::kReasonNone;
    break;

  case StopInfo::kEventStop:
    stop.event = StopCode::kSignal;
    stop.reason = info.reason;
#if !defined(OS_WIN32)
    stop.signal = info.signal;
#endif
    break;

  case StopInfo::kEventExit:
    DS2ASSERT(stop.reason == StopInfo::kReasonNone);
    stop.event = StopCode::kCleanExit;
    stop.status = info.status;
    readRegisters = false;
    break;

#if !defined(OS_WIN32)
  case StopInfo::kEventKill:
    DS2ASSERT(stop.reason == StopInfo::kReasonNone);
    stop.event = StopCode::kSignalExit;
    stop.signal = info.signal;
    readRegisters = false;
    break;
#endif
  }

  if (readRegisters) {
    stop.threadName = Platform::GetThreadName(stop.ptid.pid, stop.ptid.tid);
    ErrorCode error = thread->readCPUState(state);
    if (error != kSuccess)
      return error;
    state.getStopGPState(stop.registers,
                         session.mode() == kCompatibilityModeLLDB);
  }

  _process->enumerateThreads(
      [&](Thread *thread) { stop.threads.insert(thread->tid()); });

  return kSuccess;
}

ErrorCode DebugSessionImpl::onQueryThreadStopInfo(Session &session,
                                                  ProcessThreadId const &ptid,
                                                  StopCode &stop) const {
  Thread *thread = findThread(ptid);
  if (thread == nullptr)
    return kErrorProcessNotFound;

  return queryStopCode(session, ptid, stop);
}

ErrorCode DebugSessionImpl::onQueryThreadList(Session &, ProcessId pid,
                                              ThreadId lastTid,
                                              ThreadId &tid) const {
  if (_process == nullptr)
    return kErrorProcessNotFound;

  switch (lastTid) {
  case kAllThreadId:
    _threadIndex = 0;
    _process->getThreadIds(_tids);
    break;

  case kAnyThreadId:
    _threadIndex++;
    break;

  default:
    return kErrorInvalidArgument;
  }

  if (_threadIndex >= _tids.size())
    return kErrorNotFound;

  tid = _tids[_threadIndex];
  return kSuccess;
}

ErrorCode DebugSessionImpl::onQueryCurrentThread(Session &,
                                                 ProcessThreadId &ptid) const {
  if (_process == nullptr)
    return kErrorProcessNotFound;

  Thread *thread = _process->currentThread();
  if (thread == nullptr)
    return kErrorProcessNotFound;

  ptid.pid = _process->pid();
  ptid.tid = thread->tid();
  return kSuccess;
}

ErrorCode DebugSessionImpl::onThreadIsAlive(Session &session,
                                            ProcessThreadId const &ptid) {
  if (_process == nullptr)
    return kErrorProcessNotFound;

  Thread *thread = findThread(ptid);
  if (thread == nullptr)
    return kErrorProcessNotFound;

  if (thread->state() == Thread::kTerminated)
    return kErrorInvalidArgument;

  return kSuccess;
}

ErrorCode DebugSessionImpl::onQueryAttached(Session &, ProcessId pid,
                                            bool &attachedProcess) const {
  if (_process == nullptr)
    return kErrorProcessNotFound;
  if (pid != kAnyProcessId && pid != kAllProcessId && pid != _process->pid())
    return kErrorProcessNotFound;

  attachedProcess = _process->attached();
  return kSuccess;
}

ErrorCode DebugSessionImpl::onQueryProcessInfo(Session &,
                                               ProcessInfo &info) const {
  if (_process == nullptr)
    return kErrorProcessNotFound;
  else
    return _process->getInfo(info);
}

ErrorCode DebugSessionImpl::onQueryRegisterInfo(Session &, uint32_t regno,
                                                RegisterInfo &info) const {
  Architecture::LLDBRegisterInfo reginfo;
  Architecture::LLDBDescriptor const *desc =
      _process->getLLDBRegistersDescriptor();

  if (!Architecture::LLDBGetRegisterInfo(*desc, regno, reginfo))
    return kErrorInvalidArgument;

  if (reginfo.SetName != nullptr) {
    info.setName = reginfo.SetName;
  }

  if (reginfo.Def->LLDBName != nullptr) {
    info.registerName = reginfo.Def->LLDBName;
  } else {
    info.registerName = reginfo.Def->Name;
  }

  if (reginfo.Def->AlternateName != nullptr) {
    info.alternateName = reginfo.Def->AlternateName;
  }
  if (reginfo.Def->GenericName != nullptr) {
    info.genericName = reginfo.Def->GenericName;
  }

  info.bitSize = reginfo.Def->BitSize;
  info.byteOffset = reginfo.Def->LLDBOffset;
  info.gccRegisterIndex = reginfo.Def->GCCRegisterNumber;
  info.dwarfRegisterIndex = reginfo.Def->DWARFRegisterNumber;

  if (reginfo.Def->Format == Architecture::kFormatVector) {
    info.encoding = RegisterInfo::kEncodingVector;
    switch (reginfo.Def->LLDBVectorFormat) {
    case Architecture::kLLDBVectorFormatUInt8:
      info.format = RegisterInfo::kFormatVectorUInt8;
      break;
    case Architecture::kLLDBVectorFormatSInt8:
      info.format = RegisterInfo::kFormatVectorSInt8;
      break;
    case Architecture::kLLDBVectorFormatUInt16:
      info.format = RegisterInfo::kFormatVectorUInt16;
      break;
    case Architecture::kLLDBVectorFormatSInt16:
      info.format = RegisterInfo::kFormatVectorSInt16;
      break;
    case Architecture::kLLDBVectorFormatUInt32:
      info.format = RegisterInfo::kFormatVectorUInt32;
      break;
    case Architecture::kLLDBVectorFormatSInt32:
      info.format = RegisterInfo::kFormatVectorSInt32;
      break;
    case Architecture::kLLDBVectorFormatUInt128:
      info.format = RegisterInfo::kFormatVectorUInt128;
      break;
    case Architecture::kLLDBVectorFormatFloat32:
      info.format = RegisterInfo::kFormatVectorFloat32;
      break;
    default:
      info.format = RegisterInfo::kFormatVectorUInt8;
      break;
    }
  } else if (reginfo.Def->Format == Architecture::kFormatFloat) {
    info.encoding = RegisterInfo::kEncodingIEEE754;
    info.format = RegisterInfo::kFormatFloat;
  } else {
    switch (reginfo.Def->Encoding) {
    case Architecture::kEncodingUInteger:
      info.encoding = RegisterInfo::kEncodingUInt;
      break;
    case Architecture::kEncodingSInteger:
      info.encoding = RegisterInfo::kEncodingSInt;
      break;
    case Architecture::kEncodingIEEESingle:
    case Architecture::kEncodingIEEEDouble:
    case Architecture::kEncodingIEEEExtended:
      info.encoding = RegisterInfo::kEncodingIEEE754;
      break;
    default:
      info.encoding = RegisterInfo::kEncodingUInt;
      break;
    }

    switch (reginfo.Def->Format) {
    case Architecture::kFormatBinary:
      info.format = RegisterInfo::kFormatBinary;
      break;
    case Architecture::kFormatDecimal:
      info.format = RegisterInfo::kFormatDecimal;
      break;
    case Architecture::kFormatHexadecimal:
    default:
      info.format = RegisterInfo::kFormatHex;
      break;
    }
  }

  if (reginfo.Def->ContainerRegisters != nullptr) {
    info.containerRegisters.clear();
    for (size_t n = 0; reginfo.Def->ContainerRegisters[n] != nullptr; n++) {
      info.containerRegisters.push_back(
          reginfo.Def->ContainerRegisters[n]->LLDBRegisterNumber);
    }
  }

  if (reginfo.Def->InvalidatedRegisters != nullptr) {
    info.invalidateRegisters.clear();
    for (size_t n = 0; reginfo.Def->InvalidatedRegisters[n] != nullptr; n++) {
      info.invalidateRegisters.push_back(
          reginfo.Def->InvalidatedRegisters[n]->LLDBRegisterNumber);
    }
  }

  return kSuccess;
}

ErrorCode
DebugSessionImpl::onQuerySharedLibrariesInfoAddress(Session &,
                                                    Address &address) const {
  if (_process == nullptr)
    return kErrorProcessNotFound;

  return _process->getSharedLibraryInfoAddress(address);
}

ErrorCode DebugSessionImpl::onXferRead(Session &session,
                                       std::string const &object,
                                       std::string const &annex,
                                       uint64_t offset, uint64_t length,
                                       std::string &buffer, bool &last) {
  DS2LOG(Info, "object='%s' annex='%s' offset=%#" PRIx64 " length=%#" PRIx64,
         object.c_str(), annex.c_str(), offset, length);

  // TODO Split these generators into appropriate functions
  if (object == "features") {
    if (session.mode() == kCompatibilityModeLLDB) {
      Architecture::LLDBDescriptor const *desc =
          _process->getLLDBRegistersDescriptor();
      if (annex == "target.xml") {
        buffer = Architecture::LLDBGenerateXMLMain(*desc).substr(offset);
      } else {
        std::ostringstream ss;
        ss << Architecture::GenerateXMLHeader();
        ss << "<feature>" << std::endl;
        RegisterInfo info;
        std::string lastSet;
        int setNum = 0;
        for (uint32_t regno = 0;
             onQueryRegisterInfo(session, regno, info) == kSuccess; ++regno) {
          if (info.setName != annex) {
            if (info.setName != lastSet) {
              lastSet = info.setName;
              setNum++;
            }
            continue;
          }
          ss << '\t' << info.encode(setNum) << '\n';
        }
        ss << "</feature>" << std::endl;
        buffer = ss.str().substr(offset);
      }
    } else {
      Architecture::GDBDescriptor const *desc =
          _process->getGDBRegistersDescriptor();
      if (annex == "target.xml") {
        buffer = Architecture::GDBGenerateXMLMain(*desc).substr(offset);
      } else {
        buffer = Architecture::GDBGenerateXMLFeatureByFileName(*desc, annex)
                     .substr(offset);
      }
    }
  } else if (object == "auxv") {
    ErrorCode error = _process->getAuxiliaryVector(buffer);
    if (error != kSuccess)
      return error;

    buffer = buffer.substr(offset);
  } else if (object == "threads") {
    std::ostringstream ss;

    ss << "<threads>" << std::endl;

    _process->enumerateThreads([&](Thread *thread) {
      ss << "<thread "
         << "id=\"p" << std::hex << _process->pid() << '.' << std::hex
         << thread->tid() << "\" "
         << "core=\"" << std::dec << thread->core() << "\""
         << "/>" << std::endl;
    });

    ss << "</threads>" << std::endl;

    buffer = ss.str().substr(offset);
  } else if (object == "libraries") {
    std::ostringstream ss;

    ss << "<library-list>" << std::endl;

    _process->enumerateSharedLibraries([&](SharedLibraryInfo const &library) {
      // Ignore the main module and move on to the next one.
      if (library.main)
        return;

      ss << "  <library name=\"" << library.path << "\">" << std::endl;
      for (auto section : library.sections)
        ss << "    <section address=\"0x" << std::hex << section << "\" />"
           << std::endl;
      ss << "  </library>" << std::endl;
    });

    ss << "</library-list>";
    buffer = ss.str().substr(offset);
  } else if (object == "libraries-svr4") {
    std::ostringstream ss;
    std::ostringstream sslibs;
    Address mainMapAddress;

    if (_process->isELFProcess()) {
      _process->enumerateSharedLibraries([&](SharedLibraryInfo const &library) {
        if (library.main) {
          mainMapAddress = library.svr4.mapAddress;
        } else {
          sslibs << "<library "
                 << "name=\"" << library.path << "\" "
                 << "lm=\""
                 << "0x" << std::hex << library.svr4.mapAddress << "\" "
                 << "l_addr=\""
                 << "0x" << std::hex << library.svr4.baseAddress << "\" "
                 << "l_ld=\""
                 << "0x" << std::hex << library.svr4.ldAddress << "\" "
                 << "/>" << std::endl;
        }
      });

      ss << "<library-list-svr4 version=\"1.0\"";
      if (mainMapAddress.valid()) {
        ss << " main-lm=\""
           << "0x" << std::hex << mainMapAddress.value() << "\"";
      }
      ss << ">" << std::endl;
      ss << sslibs.str();
      ss << "</library-list-svr4>";
      buffer = ss.str().substr(offset);
    }
  } else {
    return kErrorUnsupported;
  }

  if (buffer.length() > length) {
    buffer.resize(length);
    last = false;
  }

  return kSuccess;
}

ErrorCode DebugSessionImpl::onSetEnvironmentVariable(Session &,
                                                     std::string const &key,
                                                     std::string const &value) {
  if (!_spawner.addEnvironment(key, value))
    return kErrorInvalidArgument;

  return kSuccess;
}

ErrorCode DebugSessionImpl::onSetStdFile(Session &, int fileno,
                                         std::string const &path) {
  bool success = false;
  switch (fileno) {
  case 0:
    success = _spawner.redirectInputToFile(path);
    break;
  case 1:
    success = _spawner.redirectOutputToFile(path);
    break;
  case 2:
    success = _spawner.redirectErrorToFile(path);
    break;
  default:
    break;
  }

  if (success)
    return kSuccess;

  return kErrorInvalidArgument;
}

ErrorCode DebugSessionImpl::onReadGeneralRegisters(
    Session &, ProcessThreadId const &ptid,
    Architecture::GPRegisterValueVector &regs) {
  Thread *thread = findThread(ptid);
  if (thread == nullptr)
    return kErrorProcessNotFound;

  Architecture::CPUState state;
  ErrorCode error = thread->readCPUState(state);
  if (error != kSuccess)
    return error;

  state.getGPState(regs);

  return kSuccess;
}

ErrorCode DebugSessionImpl::onWriteGeneralRegisters(
    Session &, ProcessThreadId const &ptid, std::vector<uint64_t> const &regs) {
  Thread *thread = findThread(ptid);
  if (thread == nullptr)
    return kErrorProcessNotFound;

  Architecture::CPUState state;
  ErrorCode error = thread->readCPUState(state);
  if (error != kSuccess)
    return error;

  state.setGPState(regs);

  return thread->writeCPUState(state);
}

ErrorCode DebugSessionImpl::onSaveRegisters(Session &session,
                                            ProcessThreadId const &ptid,
                                            uint64_t &id) {
  static uint64_t counter = 1;

  Thread *thread = findThread(ptid);
  if (thread == nullptr)
    return kErrorProcessNotFound;

  Architecture::CPUState state;
  ErrorCode error = thread->readCPUState(state);
  if (error != kSuccess)
    return error;

  _savedRegisters[counter] = state;
  id = counter++;
  return kSuccess;
}

ErrorCode DebugSessionImpl::onRestoreRegisters(Session &session,
                                               ProcessThreadId const &ptid,
                                               uint64_t id) {
  Thread *thread = findThread(ptid);
  if (thread == nullptr)
    return kErrorProcessNotFound;

  auto it = _savedRegisters.find(id);
  if (it == _savedRegisters.end())
    return kErrorNotFound;

  ErrorCode error = thread->writeCPUState(it->second);
  if (error != kSuccess)
    return error;

  _savedRegisters.erase(it);

  return kSuccess;
}

ErrorCode DebugSessionImpl::onReadRegisterValue(Session &session,
                                                ProcessThreadId const &ptid,
                                                uint32_t regno,
                                                std::string &value) {
  Thread *thread = findThread(ptid);
  if (thread == nullptr)
    return kErrorProcessNotFound;

  Architecture::CPUState state;
  ErrorCode error = thread->readCPUState(state);
  if (error != kSuccess)
    return error;

  void *ptr;
  size_t length;
  bool success;

  if (session.mode() == kCompatibilityModeLLDB) {
    success = state.getLLDBRegisterPtr(regno, &ptr, &length);
  } else {
    success = state.getGDBRegisterPtr(regno, &ptr, &length);
  }

  if (!success)
    return kErrorInvalidArgument;

  value.insert(value.end(), reinterpret_cast<char *>(ptr),
               reinterpret_cast<char *>(ptr) + length);

  return kSuccess;
}

ErrorCode DebugSessionImpl::onWriteRegisterValue(Session &session,
                                                 ProcessThreadId const &ptid,
                                                 uint32_t regno,
                                                 std::string const &value) {
  Thread *thread = findThread(ptid);
  if (thread == nullptr)
    return kErrorProcessNotFound;

  Architecture::CPUState state;
  ErrorCode error = thread->readCPUState(state);
  if (error != kSuccess)
    return error;

  void *ptr;
  size_t length;
  bool success;

  if (session.mode() == kCompatibilityModeLLDB) {
    success = state.getLLDBRegisterPtr(regno, &ptr, &length);
  } else {
    success = state.getGDBRegisterPtr(regno, &ptr, &length);
  }

  if (!success)
    return kErrorInvalidArgument;

  if (value.length() != length)
    return kErrorInvalidArgument;

  std::memcpy(ptr, value.c_str(), length);

  return thread->writeCPUState(state);
}

ErrorCode DebugSessionImpl::onReadMemory(Session &, Address const &address,
                                         size_t length, std::string &data) {
  if (_process == nullptr)
    return kErrorProcessNotFound;
  else
    return _process->readMemoryBuffer(address, length, data);
}

ErrorCode DebugSessionImpl::onWriteMemory(Session &, Address const &address,
                                          std::string const &data,
                                          size_t &nwritten) {
  if (_process == nullptr)
    return kErrorProcessNotFound;
  else
    return _process->writeMemoryBuffer(address, data, &nwritten);
}

ErrorCode DebugSessionImpl::onAllocateMemory(Session &, size_t size,
                                             uint32_t permissions,
                                             Address &address) {
  uint64_t addr;
  ErrorCode error = _process->allocateMemory(size, permissions, &addr);
  if (error == kSuccess) {
    _allocations[addr] = size;
    address = addr;
  }
  return error;
}

ErrorCode DebugSessionImpl::onDeallocateMemory(Session &,
                                               Address const &address) {
  auto i = _allocations.find(address);
  if (i == _allocations.end())
    return kErrorInvalidArgument;

  ErrorCode error = _process->deallocateMemory(address, i->second);
  if (error != kSuccess)
    return error;

  _allocations.erase(i);
  return kSuccess;
}

ErrorCode
DebugSessionImpl::onQueryMemoryRegionInfo(Session &, Address const &address,
                                          MemoryRegionInfo &info) const {
  if (_process == nullptr)
    return kErrorProcessNotFound;
  else
    return _process->getMemoryRegionInfo(address, info);
}

ErrorCode
DebugSessionImpl::onSetProgramArguments(Session &,
                                        StringCollection const &args) {
  spawnProcess(args, {});
  if (_process == nullptr)
    return kErrorUnknown;

  return kSuccess;
}

ErrorCode DebugSessionImpl::onQueryLaunchSuccess(Session &, ProcessId) const {
  return kSuccess;
}

ErrorCode DebugSessionImpl::onAttach(Session &session, ProcessId pid,
                                     AttachMode mode, StopCode &stop) {
  if (_process != nullptr)
    return kErrorAlreadyExist;

  if (mode != kAttachNow)
    return kErrorInvalidArgument;

  DS2LOG(Info, "attaching to pid %" PRIu64, (uint64_t)pid);
  _process = Target::Process::Attach(pid);
  DS2LOG(Debug, "_process=%p", _process);
  if (_process == nullptr)
    return kErrorProcessNotFound;

  return queryStopCode(session, pid, stop);
}

ErrorCode
DebugSessionImpl::onResume(Session &session,
                           ThreadResumeAction::Collection const &actions,
                           StopCode &stop) {
  ErrorCode error;
  ThreadResumeAction globalAction;
  bool hasGlobalAction = false;
  std::set<Thread *> excluded;

  DS2ASSERT(_resumeSession == nullptr);
  _resumeSession = &session;
  _resumeSessionLock.unlock();

  error = _process->beforeResume();
  if (error != kSuccess)
    goto ret;

  //
  // First process all actions that specify a thread,
  // save the global and trigger it later.
  //
  for (auto const &action : actions) {
    if (action.ptid.any()) {
      if (hasGlobalAction) {
        DS2LOG(Error, "more than one global action specified");
        error = kErrorAlreadyExist;
        goto ret;
      }

      globalAction = action;
      hasGlobalAction = true;
      continue;
    }

    Thread *thread = findThread(action.ptid);
    if (thread == nullptr) {
      DS2LOG(Warning, "pid %" PRIu64 " tid %" PRIu64 " not found",
             (uint64_t)action.ptid.pid, (uint64_t)action.ptid.tid);
      continue;
    }

    if (action.action == kResumeActionContinue ||
        action.action == kResumeActionContinueWithSignal) {
      error = thread->resume(action.signal, action.address);
      if (error != kSuccess) {
        DS2LOG(Warning,
               "cannot resume pid %" PRIu64 " tid %" PRIu64 ", error=%s",
               (uint64_t)_process->pid(), (uint64_t)thread->tid(),
               Stringify::Error(error));
        continue;
      }
      excluded.insert(thread);
    } else if (action.action == kResumeActionSingleStep ||
               action.action == kResumeActionSingleStepWithSignal) {
      error = thread->step(action.signal, action.address);
      if (error != kSuccess) {
        DS2LOG(Warning, "cannot step pid %" PRIu64 " tid %" PRIu64 ", error=%s",
               (uint64_t)_process->pid(), (uint64_t)thread->tid(),
               Stringify::Error(error));
        continue;
      }
      excluded.insert(thread);
    } else {
      DS2LOG(Warning, "cannot resume pid %" PRIu64 " tid %" PRIu64
                      ", action %d not yet implemented",
             (uint64_t)_process->pid(), (uint64_t)thread->tid(), action.action);
      continue;
    }
  }

  //
  // Now trigger the global action
  //
  if (hasGlobalAction) {
    if (globalAction.action == kResumeActionContinue ||
        globalAction.action == kResumeActionContinueWithSignal) {
      if (globalAction.address.valid()) {
        DS2LOG(Warning, "global continue with address");
      }

      error = _process->resume(globalAction.signal, excluded);
      if (error != kSuccess && error != kErrorAlreadyExist) {
        DS2LOG(Warning, "cannot resume pid %" PRIu64 ", error=%s",
               (uint64_t)_process->pid(), Stringify::Error(error));
      }
    } else if (globalAction.action == kResumeActionSingleStep ||
               globalAction.action == kResumeActionSingleStepWithSignal) {
      Thread *thread = _process->currentThread();
      if (excluded.find(thread) == excluded.end()) {
        error = thread->step(globalAction.signal, globalAction.address);
        if (error != kSuccess) {
          DS2LOG(Warning,
                 "cannot step pid %" PRIu64 " tid %" PRIu64 ", error=%s",
                 (uint64_t)_process->pid(), (uint64_t)thread->tid(),
                 Stringify::Error(error));
        }
      }
    } else {
      DS2LOG(Warning,
             "cannot resume pid %" PRIu64 ", action %d not yet implemented",
             (uint64_t)_process->pid(), globalAction.action);
    }
  }

  //
  // If kErrorAlreadyExist is set, then a signal is already pending.
  //
  if (error != kErrorAlreadyExist) {
    //
    // Wait for the next signal
    //
    error = _process->wait();
    if (error != kSuccess)
      goto ret;
  }

  error = _process->afterResume();
  if (error != kSuccess)
    goto ret;

  error = queryStopCode(
      session,
      ProcessThreadId(_process->pid(), _process->currentThread()->tid()), stop);

  if (stop.event == StopCode::kCleanExit || stop.event == StopCode::kSignalExit)
    _spawner.flushAndExit();

ret:
  _resumeSessionLock.lock();
  _resumeSession = nullptr;
  return error;
}

ErrorCode DebugSessionImpl::onDetach(Session &, ProcessId, bool stopped) {
  ErrorCode error;

  BreakpointManager *bpm = _process->breakpointManager();
  if (bpm != nullptr) {
    bpm->clear();
  }

  if (stopped) {
    error = _process->suspend();
    if (error != kSuccess)
      return error;
  }

  return _process->detach();
}

ErrorCode DebugSessionImpl::onTerminate(Session &session,
                                        ProcessThreadId const &ptid,
                                        StopCode &stop) {
  ErrorCode error;

  error = _process->terminate();
  if (error != kSuccess) {
    DS2LOG(Error, "couldn't terminate process");
    return error;
  }

  error = _process->wait();
  if (error != kSuccess) {
    DS2LOG(Error, "couldn't wait for process termination");
    return error;
  }

  return queryStopCode(session, _process->pid(), stop);
}

//
// For LLDB we need to support breakpoints through the breakpoint manager
// because LLDB is unable to handle software breakpoints.
// In GDB mode we let GDB handle the breakpoints.
//
ErrorCode DebugSessionImpl::onInsertBreakpoint(
    Session &session, BreakpointType type, Address const &address,
    uint32_t size, StringCollection const &, StringCollection const &, bool) {
  //    if (session.mode() != kCompatibilityModeLLDB)
  //        return kErrorUnsupported;

  if (type != kSoftwareBreakpoint)
    return kErrorUnsupported;

  BreakpointManager *bpm = _process->breakpointManager();
  if (bpm == nullptr)
    return kErrorUnsupported;

  return bpm->add(address, BreakpointManager::kTypePermanent, size);
}

ErrorCode DebugSessionImpl::onRemoveBreakpoint(Session &session,
                                               BreakpointType type,
                                               Address const &address,
                                               uint32_t size) {
  //    if (session.mode() != kCompatibilityModeLLDB)
  //        return kErrorUnsupported;

  if (type != kSoftwareBreakpoint)
    return kErrorUnsupported;

  BreakpointManager *bpm = _process->breakpointManager();
  if (bpm == nullptr)
    return kErrorUnsupported;

  return bpm->remove(address);
}

ErrorCode DebugSessionImpl::spawnProcess(StringCollection const &args,
                                         EnvironmentBlock const &env) {
  DS2LOG(Debug, "spawning process with args:");
  for (auto const &arg : args)
    DS2LOG(Debug, "  %s", arg.c_str());

  _spawner.setExecutable(args[0]);
  _spawner.setArguments(StringCollection(args.begin() + 1, args.end()));

  if (!env.empty()) {
    DS2LOG(Debug, "and with environment:");
    for (auto const &val : env)
      DS2LOG(Debug, "  %s=%s", val.first.c_str(), val.second.c_str());

    _spawner.setEnvironment(env);
  }

  auto outputDelegate = [this](void *buf, size_t size) {
    const char *cbuf = static_cast<char *>(buf);
    for (size_t i = 0; i < size; ++i) {
      this->_consoleBuffer += cbuf[i];
      if (cbuf[i] == '\n') {
        _resumeSessionLock.lock();
        DS2ASSERT(_resumeSession != nullptr);
        std::string data = "O";
        data += StringToHex(this->_consoleBuffer);
        _consoleBuffer.clear();
        this->_resumeSession->send(data);
        _resumeSessionLock.unlock();
      }
    }
  };

  _spawner.redirectOutputToDelegate(outputDelegate);
  _spawner.redirectErrorToDelegate(outputDelegate);

  _process = ds2::Target::Process::Create(_spawner);
  if (_process == nullptr) {
    DS2LOG(Error, "cannot execute '%s'", args[0].c_str());
    return kErrorUnknown;
  }

  return kSuccess;
}

ErrorCode DebugSessionImpl::fetchStopInfoForAllThreads(
    Session &session, std::vector<StopCode> &stops, StopCode &processStop) {
  ErrorCode error =
      onQueryThreadStopInfo(session, ProcessThreadId(), processStop);
  if (error != kSuccess) {
    return error;
  }

  for (auto const &tid : processStop.threads) {
    StopCode stop;
    onQueryThreadStopInfo(session, ProcessThreadId(kAnyProcessId, tid), stop);
    stops.push_back(stop);
  }

  return kSuccess;
}

ErrorCode DebugSessionImpl::createThreadsStopInfo(Session &session,
                                                  JSArray &threadsStopInfo) {
  StopCode processStop;
  std::vector<StopCode> stops;
  ErrorCode error = fetchStopInfoForAllThreads(session, stops, processStop);
  if (error != kSuccess) {
    return error;
  }

  for (auto const &stop : stops) {
    threadsStopInfo.append(stop.encodeBriefJson());
  }
  return kSuccess;
}
}
}
