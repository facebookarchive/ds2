//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Host/POSIX/Platform.h"

#include <limits.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

// TODO HAVE_ENDIAN_H, HAVE_SYS_ENDIAN_H
#if defined(__linux__)
#include <endian.h>
#else
#include <sys/endian.h>
#endif

using ds2::Host::POSIX::Platform;

void Platform::Initialize() {
  // Nothing to do here.
}

ds2::CPUType Platform::GetCPUType() {
#if defined(__arm__) || defined(__aarc64__)
  return (sizeof(void *) == 8) ? kCPUTypeARM64 : kCPUTypeARM;
#elif defined(__i386__) || defined(__x86_64__)
  return (sizeof(void *) == 8) ? kCPUTypeX86_64 : kCPUTypeI386;
#elif defined(__powerpc__) || defined(__powerpc64__)
  return (sizeof(void *) == 8) ? kCPUTypePOWERPC64 : kCPUTypePOWERPC;
#elif defined(__mips__) || defined(__mips64__)
  return (sizeof(void *) == 8) ? kCPUTypeMIPS64 : kCPUTypeMIPS;
#elif defined(__sparc__) || defined(__sparc64__)
  return (sizeof(void *) == 8) ? kCPUTypeSPARC64 : kCPUTypeSPARC;
#elif defined(__hppa__) || defined(__hppa64__)
  return (sizeof(void *) == 8) ? kCPUTypeHPPA64 : kCPUTypeHPPA;
#elif defined(__alpha__)
  return kCPUTypeALPHA;
#elif defined(__i860__)
  return kCPUTypeI860;
#elif defined(__m68k__)
  return kCPUTypeMC680x0;
#elif defined(__vax__)
  return kCPUTypeVAX;
#elif defined(__m88k__)
  return kCPUTypeMC88000;
#else
#error "I don't know your architecture."
#endif
}

ds2::CPUSubType Platform::GetCPUSubType() {
#if defined(__arm__)
#if defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) ||                     \
    defined(__ARM_ARCH_7R__)
  return kCPUSubTypeARM_V7;
#elif defined(__ARM_ARCH_7EM__)
  return kCPUSubTypeARM_V7EM;
#elif defined(__ARM_ARCH_7M__)
  return kCPUSubTypeARM_V7M;
#endif
#endif
  return kCPUSubTypeInvalid;
}

char const *Platform::GetOSTypeName() { return "posix"; }

char const *Platform::GetOSVendorName() { return "unknown"; }

char const *Platform::GetOSVersion() { return nullptr; }

char const *Platform::GetOSBuild() { return nullptr; }

char const *Platform::GetOSKernelPath() { return nullptr; }

char const *Platform::GetHostName() {
  static const int kHostNameMax = 255;
  static char sHostName[kHostNameMax + 1] = {'\0'};
  if (sHostName[0] == '\0') {
    ::gethostname(sHostName, kHostNameMax);
  }
  return sHostName;
}

ds2::Endian Platform::GetEndian() {
#if defined(__LITTLE_ENDIAN__) || (__BYTE_ORDER == __LITTLE_ENDIAN) ||         \
    (BYTE_ORDER == LITTLE_ENDIAN)
  return kEndianLittle;
#elif defined(__BIG_ENDIAN__) || (__BYTE_ORDER == __BIG_ENDIAN) ||             \
    (BYTE_ORDER == BIG_ENDIAN)
  return kEndianBig;
#elif defined(__PDP_ENDIAN__) || (__BYTE_ORDER == __PDP_ENDIAN) ||             \
    (BYTE_ORDER == PDP_ENDIAN)
  return kEndianPDP;
#else
  return kEndianUnknown;
#endif
}

size_t Platform::GetPointerSize() { return sizeof(void *); }

bool Platform::GetUserName(UserId const &uid, std::string &name) {
  struct passwd *pwd = ::getpwuid(uid);
  if (pwd == nullptr)
    return false;

  name = pwd->pw_name;
  return true;
}

bool Platform::GetGroupName(GroupId const &gid, std::string &name) {
  struct group *grp = ::getgrgid(gid);
  if (grp == nullptr)
    return false;

  name = grp->gr_name;
  return true;
}

bool Platform::IsFilePresent(std::string const &path) {
  if (path.empty())
    return false;

  return (::access(path.c_str(), F_OK) == 0);
}

ds2::ProcessId Platform::GetCurrentProcessId() { return ::getpid(); }
