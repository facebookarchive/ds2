//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/GDBRemote/DebugSessionImpl.h"
#include "DebugServer2/Core/HardwareBreakpointManager.h"
#include "DebugServer2/Core/SoftwareBreakpointManager.h"
#include "DebugServer2/GDBRemote/Session.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/HexValues.h"
#include "DebugServer2/Utils/Log.h"
#include "DebugServer2/Utils/Paths.h"
#include "DebugServer2/Utils/Stringify.h"

#include <iomanip>
#include <sstream>

using ds2::Host::Platform;
using ds2::Target::Thread;
using ds2::Utils::Stringify;

namespace ds2 {
namespace GDBRemote {

DebugSessionImplBase::DebugSessionImplBase(StringCollection const &args,
                                           EnvironmentBlock const &env)
    : DummySessionDelegateImpl(), _resumeSession(nullptr) {
  DS2ASSERT(args.size() >= 1);
  _resumeSessionLock.lock();
  spawnProcess(args, env);
}

DebugSessionImplBase::DebugSessionImplBase(int attachPid)
    : DummySessionDelegateImpl(), _resumeSession(nullptr) {
  _resumeSessionLock.lock();
  _process = ds2::Target::Process::Attach(attachPid);
  if (_process == nullptr)
    DS2LOG(Fatal, "cannot attach to pid %d", attachPid);
}

DebugSessionImplBase::DebugSessionImplBase()
    : DummySessionDelegateImpl(), _process(nullptr), _resumeSession(nullptr) {
  _resumeSessionLock.lock();
}

DebugSessionImplBase::~DebugSessionImplBase() {
  _resumeSessionLock.unlock();
  delete _process;
}

size_t DebugSessionImplBase::getGPRSize() const {
  if (_process == nullptr)
    return 0;

  ProcessInfo info;
  if (_process->getInfo(info) != kSuccess)
    return 0;

  return info.pointerSize << 3;
}

ErrorCode DebugSessionImplBase::onInterrupt(Session &) {
  return _process->interrupt();
}

ErrorCode DebugSessionImplBase::onQuerySupported(
    Session &session, Feature::Collection const &remoteFeatures,
    Feature::Collection &localFeatures) const {
  for (auto feature : remoteFeatures) {
    DS2LOG(Debug, "gdb feature: %s", feature.name.c_str());
  }

  // TODO PacketSize should be respected
  localFeatures.push_back(std::string("qEcho+"));
  localFeatures.push_back(std::string("PacketSize=3fff"));
  localFeatures.push_back(std::string("QStartNoAckMode+"));
  localFeatures.push_back(std::string("qXfer:features:read+"));
#if defined(OS_LINUX) || defined(OS_FREEBSD)
  localFeatures.push_back(std::string("qXfer:auxv:read+"));
  localFeatures.push_back(std::string("qXfer:libraries-svr4:read+"));
#elif defined(OS_WIN32)
  localFeatures.push_back(std::string("qXfer:libraries:read+"));
#endif
  localFeatures.push_back(std::string("QListThreadsInStopReply+"));
  localFeatures.push_back(std::string("QPassSignals+"));

  if (session.mode() != kCompatibilityModeLLDB) {
    localFeatures.push_back(std::string("ConditionalBreakpoints-"));
    localFeatures.push_back(std::string("BreakpointCommands+"));
    localFeatures.push_back(std::string("multiprocess+"));
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
    localFeatures.push_back(std::string("qXfer:osdata:read+"));
    localFeatures.push_back(std::string("qXfer:threads:read+"));
    // Disable unsupported tracepoints
    localFeatures.push_back(std::string("Qbtrace:bts-"));
    localFeatures.push_back(std::string("Qbtrace:off-"));
    localFeatures.push_back(std::string("tracenz-"));
    localFeatures.push_back(std::string("ConditionalTracepoints-"));
    localFeatures.push_back(std::string("TracepointSource-"));
    localFeatures.push_back(std::string("EnableDisableTracepoints-"));
  }

  return kSuccess;
}

ErrorCode DebugSessionImplBase::onPassSignals(Session &session,
                                              std::vector<int> const &signals) {
#if defined(OS_POSIX)
  _process->resetSignalPass();
  for (int signo : signals) {
    DS2LOG(Debug, "passing signal %d", signo);
    _process->setSignalPass(signo, true);
  }
  return kSuccess;
#else
  return kErrorUnsupported;
#endif
}

ErrorCode
DebugSessionImplBase::onProgramSignals(Session &session,
                                       std::vector<int> const &signals) {
#if defined(OS_POSIX)
  for (int signo : signals) {
    DS2LOG(Debug, "programming signal %d", signo);
    _process->setSignalPass(signo, false);
  }
  return kSuccess;
#else
  return kErrorUnsupported;
#endif
}

ErrorCode DebugSessionImplBase::onNonStopMode(Session &session, bool enable) {
  if (enable)
    return kErrorUnsupported; // TODO support non-stop mode

  return kSuccess;
}

Thread *DebugSessionImplBase::findThread(ProcessThreadId const &ptid) const {
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

ErrorCode DebugSessionImplBase::queryStopInfo(Session &session, Thread *thread,
                                              StopInfo &stop) const {
  DS2ASSERT(thread != nullptr);

  // Directly copy the fields that are common between ds2::StopInfo and
  // ds2::GDBRemote::StopInfo.
  stop = thread->stopInfo();

  stop.ptid.pid = thread->process()->pid();
  stop.ptid.tid = thread->tid();

  // Modify and augment the information we got from thread->stopInfo() to make
  // it a full GDBRemote::StopInfo.
  switch (stop.event) {
  case StopInfo::kEventNone:
    stop.event = StopInfo::kEventStop;
    stop.reason = StopInfo::kReasonNone;
    DS2_FALLTHROUGH;

  // fall-through from kEventNone.
  case StopInfo::kEventStop: {
    // Thread name won't be available if the process has exited or has been
    // killed.
    stop.threadName = Platform::GetThreadName(stop.ptid.pid, stop.ptid.tid);

    Architecture::CPUState state;
    CHK(thread->readCPUState(state));
    state.getStopGPState(stop.registers,
                         session.mode() == kCompatibilityModeLLDB);
  } break;

  case StopInfo::kEventExit:
  case StopInfo::kEventKill:
    DS2ASSERT(stop.reason == StopInfo::kReasonNone);
    break;

  default:
    DS2BUG("impossible StopInfo event: %s", Stringify::StopEvent(stop.event));
  }

  _process->enumerateThreads(
      [&](Thread *thread) { stop.threads.insert(thread->tid()); });

  return kSuccess;
}

ErrorCode DebugSessionImplBase::queryStopInfo(Session &session,
                                              ProcessThreadId const &ptid,
                                              StopInfo &stop) const {
  Thread *thread = findThread(ptid);
  if (thread == nullptr) {
    return kErrorInvalidArgument;
  }

  return queryStopInfo(session, thread, stop);
}

ErrorCode DebugSessionImplBase::onQueryThreadStopInfo(
    Session &session, ProcessThreadId const &ptid, StopInfo &stop) const {
  Thread *thread = findThread(ptid);
  if (thread == nullptr)
    return kErrorProcessNotFound;

  return queryStopInfo(session, ptid, stop);
}

ErrorCode DebugSessionImplBase::onQueryThreadList(Session &, ProcessId pid,
                                                  ThreadId lastTid,
                                                  ThreadId &tid) const {
  if (_process == nullptr)
    return kErrorProcessNotFound;

  std::vector<ThreadId> tids;

  if (lastTid == kAllThreadId) {
    _process->getThreadIds(tids);
    _threadIterationState.vals = tids;
    _threadIterationState.it = _threadIterationState.vals.begin();
  } else if (lastTid != kAnyThreadId) {
    return kErrorInvalidArgument;
  }

  if (_threadIterationState.it == _threadIterationState.vals.end())
    return kErrorNotFound;

  tid = *_threadIterationState.it++;
  return kSuccess;
}

ErrorCode
DebugSessionImplBase::onQueryCurrentThread(Session &,
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

ErrorCode DebugSessionImplBase::onThreadIsAlive(Session &session,
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

ErrorCode DebugSessionImplBase::onQueryAttached(Session &, ProcessId pid,
                                                bool &attachedProcess) const {
  if (_process == nullptr)
    return kErrorProcessNotFound;
  if (pid != kAnyProcessId && pid != kAllProcessId && pid != _process->pid())
    return kErrorProcessNotFound;

  attachedProcess = _process->attached();
  return kSuccess;
}

ErrorCode DebugSessionImplBase::onQueryProcessInfo(Session &,
                                                   ProcessInfo &info) const {
  if (_process == nullptr)
    return kErrorProcessNotFound;
  else
    return _process->getInfo(info);
}

ErrorCode
DebugSessionImplBase::onQueryHardwareWatchpointCount(Session &,
                                                     size_t &count) const {
  if (_process == nullptr) {
    return kErrorProcessNotFound;
  }

  HardwareBreakpointManager *bpm = _process->hardwareBreakpointManager();
  if (bpm == nullptr) {
    count = 0;
  } else {
    count = bpm->maxWatchpoints();
  }

  return kSuccess;
}

ErrorCode DebugSessionImplBase::onQueryRegisterInfo(Session &, uint32_t regno,
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
  info.ehframeRegisterIndex = reginfo.Def->EHFrameRegisterNumber;
  info.dwarfRegisterIndex = reginfo.Def->DWARFRegisterNumber;
  info.regno = regno;

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

  info.containerRegisters.clear();
  if (reginfo.Def->ContainerRegisters != nullptr) {
    for (size_t n = 0; reginfo.Def->ContainerRegisters[n] != nullptr; n++) {
      info.containerRegisters.push_back(
          reginfo.Def->ContainerRegisters[n]->LLDBRegisterNumber);
    }
  }

  info.invalidateRegisters.clear();
  if (reginfo.Def->InvalidatedRegisters != nullptr) {
    for (size_t n = 0; reginfo.Def->InvalidatedRegisters[n] != nullptr; n++) {
      info.invalidateRegisters.push_back(
          reginfo.Def->InvalidatedRegisters[n]->LLDBRegisterNumber);
    }
  }

  return kSuccess;
}

ErrorCode DebugSessionImplBase::onQuerySharedLibrariesInfoAddress(
    Session &, Address &address) const {
  if (_process == nullptr) {
    return kErrorProcessNotFound;
  }

#if defined(OS_POSIX)
  return _process->getSharedLibraryInfoAddress(address);
#else
  return kErrorUnsupported;
#endif
}

ErrorCode DebugSessionImplBase::onQueryFileLoadAddress(
    Session &session, std::string const &file_path, Address &address) {
  if (_process == nullptr) {
    return kErrorProcessNotFound;
  }

  CHK(_process->enumerateMappedFiles([&](MappedFileInfo const &file) {
    if (file.path == file_path ||
        ds2::Utils::Basename(file.path) == file_path) {
      address = Address(file.baseAddress);
    }
  }));
  if (!address.valid()) {
    return kErrorNotFound;
  }
  return kSuccess;
}

ErrorCode DebugSessionImplBase::onXferRead(Session &session,
                                           std::string const &object,
                                           std::string const &annex,
                                           uint64_t offset, uint64_t length,
                                           std::string &buffer, bool &last) {
  DS2LOG(Debug, "object='%s' annex='%s' offset=%#" PRIx64 " length=%#" PRIx64,
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
    CHK(_process->getAuxiliaryVector(buffer));

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

      ss << "  <library name=\"" << ds2::Utils::Basename(library.path) << "\">"
         << std::endl;
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
  } else {
    return kErrorUnsupported;
  }

  if (buffer.length() > length) {
    buffer.resize(length);
    last = false;
  }

  return kSuccess;
}

ErrorCode DebugSessionImplBase::onSetEnvironmentVariable(
    Session &, std::string const &key, std::string const &value) {
  if (!_spawner.addEnvironment(key, value))
    return kErrorInvalidArgument;

  return kSuccess;
}

ErrorCode DebugSessionImplBase::onSetStdFile(Session &, int fileno,
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

ErrorCode DebugSessionImplBase::onReadGeneralRegisters(
    Session &, ProcessThreadId const &ptid,
    Architecture::GPRegisterValueVector &regs) {
  Thread *thread = findThread(ptid);
  if (thread == nullptr)
    return kErrorProcessNotFound;

  Architecture::CPUState state;
  CHK(thread->readCPUState(state));

  state.getGPState(regs);

  return kSuccess;
}

ErrorCode DebugSessionImplBase::onWriteGeneralRegisters(
    Session &, ProcessThreadId const &ptid, std::vector<uint64_t> const &regs) {
  Thread *thread = findThread(ptid);
  if (thread == nullptr)
    return kErrorProcessNotFound;

  Architecture::CPUState state;
  CHK(thread->readCPUState(state));

  state.setGPState(regs);

  return thread->writeCPUState(state);
}

ErrorCode DebugSessionImplBase::onSaveRegisters(Session &session,
                                                ProcessThreadId const &ptid,
                                                uint64_t &id) {
  static uint64_t counter = 1;

  Thread *thread = findThread(ptid);
  if (thread == nullptr)
    return kErrorProcessNotFound;

  Architecture::CPUState state;
  CHK(thread->readCPUState(state));

  _savedRegisters[counter] = state;
  id = counter++;
  return kSuccess;
}

ErrorCode DebugSessionImplBase::onRestoreRegisters(Session &session,
                                                   ProcessThreadId const &ptid,
                                                   uint64_t id) {
  Thread *thread = findThread(ptid);
  if (thread == nullptr)
    return kErrorProcessNotFound;

  auto it = _savedRegisters.find(id);
  if (it == _savedRegisters.end())
    return kErrorNotFound;

  CHK(thread->writeCPUState(it->second));

  _savedRegisters.erase(it);

  return kSuccess;
}

ErrorCode DebugSessionImplBase::onReadRegisterValue(Session &session,
                                                    ProcessThreadId const &ptid,
                                                    uint32_t regno,
                                                    std::string &value) {
  Thread *thread = findThread(ptid);
  if (thread == nullptr)
    return kErrorProcessNotFound;

  Architecture::CPUState state;
  CHK(thread->readCPUState(state));

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

ErrorCode DebugSessionImplBase::onWriteRegisterValue(
    Session &session, ProcessThreadId const &ptid, uint32_t regno,
    std::string const &value) {
  Thread *thread = findThread(ptid);
  if (thread == nullptr)
    return kErrorProcessNotFound;

  Architecture::CPUState state;
  CHK(thread->readCPUState(state));

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

ErrorCode DebugSessionImplBase::onReadMemory(Session &, Address const &address,
                                             size_t length, ByteVector &data) {
  if (_process == nullptr)
    return kErrorProcessNotFound;
  else
    return _process->readMemoryBuffer(address, length, data);
}

ErrorCode DebugSessionImplBase::onWriteMemory(Session &, Address const &address,
                                              ByteVector const &data,
                                              size_t &nwritten) {
  if (_process == nullptr)
    return kErrorProcessNotFound;
  else
    return _process->writeMemoryBuffer(address, data, &nwritten);
}

ErrorCode DebugSessionImplBase::onAllocateMemory(Session &, size_t size,
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

ErrorCode DebugSessionImplBase::onDeallocateMemory(Session &,
                                                   Address const &address) {
  auto i = _allocations.find(address);
  if (i == _allocations.end())
    return kErrorInvalidArgument;

  CHK(_process->deallocateMemory(address, i->second));

  _allocations.erase(i);
  return kSuccess;
}

ErrorCode
DebugSessionImplBase::onQueryMemoryRegionInfo(Session &, Address const &address,
                                              MemoryRegionInfo &info) const {
  if (_process == nullptr)
    return kErrorProcessNotFound;
  else
    return _process->getMemoryRegionInfo(address, info);
}

ErrorCode
DebugSessionImplBase::onSetProgramArguments(Session &,
                                            StringCollection const &args) {
  spawnProcess(args, {});
  if (_process == nullptr)
    return kErrorUnknown;

  return kSuccess;
}

ErrorCode DebugSessionImplBase::onQueryLaunchSuccess(Session &,
                                                     ProcessId) const {
  return kSuccess;
}

ErrorCode DebugSessionImplBase::onAttach(Session &session, ProcessId pid,
                                         AttachMode mode, StopInfo &stop) {
  if (_process != nullptr)
    return kErrorAlreadyExist;

  if (mode != kAttachNow)
    return kErrorInvalidArgument;

  DS2LOG(Debug, "attaching to pid %" PRIu64, (uint64_t)pid);
  _process = Target::Process::Attach(pid);
  if (_process == nullptr) {
    return kErrorProcessNotFound;
  }

  return queryStopInfo(session, pid, stop);
}

ErrorCode
DebugSessionImplBase::onResume(Session &session,
                               ThreadResumeAction::Collection const &actions,
                               StopInfo &stop) {
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
      DS2LOG(Warning,
             "cannot resume pid %" PRIu64 " tid %" PRIu64
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

  // If kErrorAlreadyExist is set, then a signal is already pending.
  if (error != kErrorAlreadyExist) {
    bool keepGoing = true;
    while (keepGoing) {
      error = _process->wait();
      if (error != kSuccess) {
        goto ret;
      }

      auto thread = _process->currentThread();
      if (thread == nullptr) {
        break;
      }

      if (thread->stopInfo().event != StopInfo::kEventStop) {
        break;
      }

      switch (thread->stopInfo().reason) {
#if defined(OS_WIN32)
      case StopInfo::kReasonDebugOutput: {
        appendOutput(thread->stopInfo().debugString.c_str(),
                     thread->stopInfo().debugString.size());
        CHK(_process->resume());
      } break;
#endif

      case StopInfo::kReasonThreadEntry:
        CHK(_process->currentThread()->beforeResume());
        CHK(_process->currentThread()->resume());
        break;

      default:
        keepGoing = false;
        break;
      }
    }
  }

  error = _process->afterResume();
  if (error != kSuccess) {
    goto ret;
  }

  error = queryStopInfo(session, _process->currentThread(), stop);

  if (stop.event == StopInfo::kEventExit ||
      stop.event == StopInfo::kEventKill) {
    _spawner.flushAndExit();
  }

ret:
  _resumeSessionLock.lock();
  _resumeSession = nullptr;
  return error;
}

ErrorCode DebugSessionImplBase::onDetach(Session &, ProcessId, bool stopped) {
  SoftwareBreakpointManager *bpm = _process->softwareBreakpointManager();
  if (bpm != nullptr) {
    bpm->clear();
  }

  if (stopped) {
    CHK(_process->suspend());
  }

  return _process->detach();
}

ErrorCode DebugSessionImplBase::onTerminate(Session &session,
                                            ProcessThreadId const &ptid,
                                            StopInfo &stop) {
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

  return queryStopInfo(session, _process->currentThread(), stop);
}

ErrorCode DebugSessionImplBase::onExitServer(Session &session) {
  ErrorCode error = kSuccess;
  ProcessId pid = kAnyProcessId;
  StopInfo stop;

  if (_process != nullptr) {
    error = _process->attached() ? onDetach(session, pid, false)
                                 : onTerminate(session, pid, stop);
  }

  DS2LOG(Debug, "exiting ds2");
  ::exit((error == kSuccess) ? EXIT_SUCCESS : EXIT_FAILURE);
}

// For LLDB we need to support breakpoints through the breakpoint manager
// because LLDB is unable to handle software breakpoints. In GDB mode we let
// GDB handle the breakpoints.
ErrorCode DebugSessionImplBase::onInsertBreakpoint(
    Session &session, BreakpointType type, Address const &address,
    uint32_t size, StringCollection const &conditions,
    StringCollection const &commands, bool persistentCommands) {
  DS2ASSERT(conditions.empty() && commands.empty() && !persistentCommands);

  BreakpointManager *bpm = nullptr;
  BreakpointManager::Mode mode;
  switch (type) {
  case kSoftwareBreakpoint:
    bpm = _process->softwareBreakpointManager();
    mode = BreakpointManager::kModeExec;
    break;

  case kHardwareBreakpoint:
    bpm = _process->hardwareBreakpointManager();
    mode = BreakpointManager::kModeExec;
    break;

  case kReadWatchpoint:
    bpm = _process->hardwareBreakpointManager();
    mode = BreakpointManager::kModeRead;
    break;

  case kWriteWatchpoint:
    bpm = _process->hardwareBreakpointManager();
    mode = BreakpointManager::kModeWrite;
    break;

  case kAccessWatchpoint:
    bpm = _process->hardwareBreakpointManager();
    mode = static_cast<BreakpointManager::Mode>(BreakpointManager::kModeRead |
                                                BreakpointManager::kModeWrite);
    break;

  default:
    DS2BUG("impossible breakpoint type");
  }

  if (bpm == nullptr)
    return kErrorUnsupported;

  return bpm->add(address, BreakpointManager::Lifetime::Permanent, size, mode);
}

ErrorCode DebugSessionImplBase::onRemoveBreakpoint(Session &session,
                                                   BreakpointType type,
                                                   Address const &address,
                                                   uint32_t size) {
  BreakpointManager *bpm = nullptr;
  switch (type) {
  case kSoftwareBreakpoint:
    bpm = _process->softwareBreakpointManager();
    break;

  case kHardwareBreakpoint:
  case kReadWatchpoint:
  case kWriteWatchpoint:
  case kAccessWatchpoint:
    bpm = _process->hardwareBreakpointManager();
    break;

  default:
    DS2BUG("impossible breakpoint type");
  }

  if (bpm == nullptr)
    return kErrorUnsupported;

  return bpm->remove(address);
}

ErrorCode DebugSessionImplBase::spawnProcess(StringCollection const &args,
                                             EnvironmentBlock const &env) {
  bool displayArgs = args.size() > 1;
  auto it = args.begin();
  DS2LOG(Debug, "spawning process '%s'%s", (it++)->c_str(),
         displayArgs ? " with args:" : "");
  while (it != args.end()) {
    DS2LOG(Debug, "  %s", (it++)->c_str());
  }

  _spawner.setExecutable(args[0]);
  _spawner.setArguments(StringCollection(args.begin() + 1, args.end()));

  if (!env.empty()) {
    DS2LOG(Debug, "%swith environment:", displayArgs ? "and " : "");
    for (auto const &val : env) {
      DS2LOG(Debug, "  %s=%s", val.first.c_str(), val.second.c_str());
    }

    _spawner.setEnvironment(env);
  }

  auto outputDelegate = [this](void *buf, size_t size) {
    appendOutput(static_cast<char *>(buf), size);
  };

  _spawner.redirectInputToTerminal();
  _spawner.redirectOutputToDelegate(outputDelegate);
  _spawner.redirectErrorToDelegate(outputDelegate);

  _process = ds2::Target::Process::Create(_spawner);
  if (_process == nullptr) {
    DS2LOG(Error, "cannot execute '%s'", args[0].c_str());
    return kErrorUnknown;
  }

  return kSuccess;
}

void DebugSessionImplBase::appendOutput(char const *buf, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    this->_consoleBuffer += buf[i];
    if (buf[i] == '\n') {
      _resumeSessionLock.lock();
      DS2ASSERT(_resumeSession != nullptr);
      std::string data = "O";
      data += ToHex(this->_consoleBuffer);
      _consoleBuffer.clear();
      this->_resumeSession->send(data);
      _resumeSessionLock.unlock();
    }
  }
}

ErrorCode DebugSessionImplBase::onSendInput(Session &session,
                                            ByteVector const &buf) {
  return _spawner.input(buf);
}

ErrorCode DebugSessionImplBase::fetchStopInfoForAllThreads(
    Session &session, std::vector<StopInfo> &stops, StopInfo &processStop) {
  CHK(onQueryThreadStopInfo(session, ProcessThreadId(), processStop));

  for (auto const &tid : processStop.threads) {
    StopInfo stop;
    onQueryThreadStopInfo(session, ProcessThreadId(kAnyProcessId, tid), stop);
    stops.push_back(stop);
  }

  return kSuccess;
}

ErrorCode
DebugSessionImplBase::createThreadsStopInfo(Session &session,
                                            JSArray &threadsStopInfo) {
  StopInfo processStop;
  std::vector<StopInfo> stops;
  CHK(fetchStopInfoForAllThreads(session, stops, processStop));

  for (auto const &stop : stops) {
    threadsStopInfo.append(stop.encodeJson());
  }
  return kSuccess;
}
} // namespace GDBRemote
} // namespace ds2
