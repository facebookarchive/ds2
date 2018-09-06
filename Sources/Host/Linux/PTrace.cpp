//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/Linux/PTrace.h"
#include "DebugServer2/Host/Linux/ExtraWrappers.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"

#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstdio>
#include <limits>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/wait.h>

#define super ds2::Host::POSIX::PTrace

namespace ds2 {
namespace Host {
namespace Linux {

ErrorCode PTrace::wait(ProcessThreadId const &ptid, int *status) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  int stat;
  pid_t ret;
  ret = waitpid(pid, &stat, __WALL);
  if (ret < 0)
    return kErrorProcessNotFound;
  DS2ASSERT(ret == pid);

  if (status != nullptr) {
    *status = stat;
  }

  return kSuccess;
}

ErrorCode PTrace::traceMe(bool disableASLR) {
  if (disableASLR) {
    int persona = ::personality(std::numeric_limits<uint32_t>::max());
    if (::personality(persona | ADDR_NO_RANDOMIZE) == -1) {
      DS2LOG(Warning, "unable to disable ASLR, error=%s", strerror(errno));
    }
  }

  return super::traceMe(false);
}

ErrorCode PTrace::traceThat(ProcessId pid) {
  if (pid <= 0)
    return kErrorInvalidArgument;

  unsigned long traceFlags = PTRACE_O_TRACECLONE;

  //
  // Trace clone and exit events to track threads.
  //
  if (wrapPtrace(PTRACE_SETOPTIONS, pid, nullptr, traceFlags) < 0) {
    DS2LOG(Warning, "unable to set PTRACE_O_TRACECLONE on pid %d, error=%s",
           pid, strerror(errno));
    return Platform::TranslateError();
  }

  return kSuccess;
}

ErrorCode PTrace::kill(ProcessThreadId const &ptid, int signal) {
  if (!ptid.valid())
    return kErrorInvalidArgument;

  int rc;
  if (!(ptid.tid <= kAnyThreadId)) {
    if (!(ptid.pid <= kAnyProcessId)) {
      rc = ::tgkill(ptid.pid, ptid.tid, signal);
    } else {
      rc = ::tkill(ptid.tid, signal);
    }
  } else {
    rc = ::kill(ptid.pid, signal);
  }

  if (rc < 0)
    return Platform::TranslateError();

  return kSuccess;
}

ErrorCode PTrace::readBytes(ProcessThreadId const &ptid, Address const &address,
                            void *buffer, size_t length, size_t *count,
                            bool nullTerm) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  bool foundNull = false;
  size_t nread = 0;
  uintptr_t base = address;
  uintptr_t *words = (uintptr_t *)buffer;

  if (length == 0 || buffer == nullptr) {
    if (count != nullptr) {
      *count = 0;
    }
    return kSuccess;
  }

  while (length > 0) {
    union {
      uintptr_t word;
      uint8_t bytes[sizeof(uintptr_t)];
    } data;
    size_t ncopy = std::min(length, sizeof(uintptr_t));

    errno = 0;
    if (ncopy < sizeof(uintptr_t)) {
      data.word = wrapPtrace(PTRACE_PEEKDATA, pid, base + nread, nullptr);
      if (errno == 0) {
        std::memcpy(words, data.bytes, ncopy);
      }
    } else {
      *words = wrapPtrace(PTRACE_PEEKDATA, pid, base + nread, nullptr);
    }

    if (errno != 0)
      break;

    if (nullTerm && strnlen((char *)words, ncopy) < ncopy) {
      foundNull = true;
      break;
    }

    length -= ncopy;
    nread += ncopy;
    words++;
  }

  if (count != nullptr) {
    *count = nread;
  }

  if (errno != 0)
    return Platform::TranslateError();

  if (nullTerm && !foundNull)
    return kErrorNameTooLong;

  return kSuccess;
}

ErrorCode PTrace::readString(ProcessThreadId const &ptid,
                             Address const &address, std::string &str,
                             size_t length, size_t *count) {
  char buffer[length];
  ErrorCode err = readBytes(ptid, address, buffer, length, count, true);
  if (err != kSuccess)
    return err;

  str = std::string(buffer);
  return kSuccess;
}

ErrorCode PTrace::readMemory(ProcessThreadId const &ptid,
                             Address const &address, void *buffer,
                             size_t length, size_t *count) {
  return readBytes(ptid, address, buffer, length, count, false);
}

ErrorCode PTrace::writeMemory(ProcessThreadId const &ptid,
                              Address const &address, void const *buffer,
                              size_t length, size_t *count) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  size_t nwritten = 0;
  uintptr_t base = address;
  uintptr_t const *words = (uintptr_t const *)buffer;

  if (length == 0 || buffer == nullptr) {
    if (count != nullptr) {
      *count = 0;
    }
    return kSuccess;
  }

  while (length > 0) {
    union {
      uintptr_t word;
      uint8_t bytes[sizeof(uintptr_t)];
    } data;
    size_t ncopy = std::min(length, sizeof(uintptr_t));

    errno = 0;
    if (ncopy < sizeof(uintptr_t)) {
      data.word = wrapPtrace(PTRACE_PEEKDATA, pid, base + nwritten, nullptr);
      if (errno == 0) {
        std::memcpy(data.bytes, words, ncopy);
        wrapPtrace(PTRACE_POKEDATA, pid, base + nwritten, data.word);
      }
    } else {
      wrapPtrace(PTRACE_POKEDATA, pid, base + nwritten, *words);
    }

    if (errno != 0)
      break;

    length -= ncopy;
    nwritten += ncopy;
    words++;
  }

  if (errno != 0)
    return Platform::TranslateError();

  if (count != nullptr) {
    *count = nwritten;
  }

  return kSuccess;
}

ErrorCode PTrace::prepareAddressForResume(ProcessThreadId const &ptid,
                                          ProcessInfo const &pinfo,
                                          Address const &address) {
  if (address.valid()) {
    Architecture::CPUState state;
    ErrorCode error = readCPUState(ptid, pinfo, state);
    if (error != kSuccess) {
      return error;
    }

    state.setPC(address);

    error = writeCPUState(ptid, pinfo, state);
    if (error != kSuccess) {
      return error;
    }
  }

  return kSuccess;
}

ErrorCode PTrace::step(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                       int signal, Address const &address) {
#if defined(ARCH_ARM)
  DS2BUG("single stepping for ARM must be implemented in software");
#endif

  ErrorCode error = prepareAddressForResume(ptid, pinfo, address);
  if (error != kSuccess) {
    return error;
  }

  return super::step(ptid, pinfo, signal);
}

ErrorCode PTrace::resume(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                         int signal, Address const &address) {
  ErrorCode error = prepareAddressForResume(ptid, pinfo, address);
  if (error != kSuccess) {
    return error;
  }

  return super::resume(ptid, pinfo, signal);
}

ErrorCode PTrace::getSigInfo(ProcessThreadId const &ptid, siginfo_t &si) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  if (wrapPtrace(PTRACE_GETSIGINFO, pid, nullptr, &si) < 0)
    return Platform::TranslateError();

  return kSuccess;
}

ErrorCode PTrace::getEventMessage(ProcessThreadId const &ptid,
                                  unsigned long &data) {
  pid_t pid;
  CHK(ptidToPid(ptid, pid));

  if (wrapPtrace(PTRACE_GETEVENTMSG, pid, nullptr, &data) < 0)
    return Platform::TranslateError();

  return kSuccess;
}

ErrorCode PTrace::readRegisterSet(ProcessThreadId const &ptid, int regSetCode,
                                  void *buffer, size_t length) {
  struct iovec iov = {buffer, length};

  if (wrapPtrace(PTRACE_GETREGSET, ptid.validTid() ? ptid.tid : ptid.pid,
                 regSetCode, &iov) < 0)
    return Platform::TranslateError();

  // On return, the kernel modifies iov.len to return the number of bytes read.
  // This should be exactly equal to the number of bytes requested.
  DS2ASSERT(iov.iov_len == length);

  return kSuccess;
}

ErrorCode PTrace::writeRegisterSet(ProcessThreadId const &ptid, int regSetCode,
                                   void const *buffer, size_t length) {
  struct iovec iov = {const_cast<void *>(buffer), length};

  if (wrapPtrace(PTRACE_SETREGSET, ptid.validTid() ? ptid.tid : ptid.pid,
                 regSetCode, &iov) < 0)
    return Platform::TranslateError();

  return kSuccess;
}
} // namespace Linux
} // namespace Host
} // namespace ds2
