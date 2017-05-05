//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Target/POSIX/ELFProcess.h"
#include "DebugServer2/Support/POSIX/ELFSupport.h"

#include <dirent.h>
#include <elf.h>
#include <limits>
#include <link.h>

#if defined(OS_FREEBSD)
#include <machine/elf.h>
typedef Elf32_Auxinfo Elf32_auxv_t;
typedef Elf64_Auxinfo Elf64_auxv_t;
#endif

using ds2::Support::ELFSupport;

#define super ds2::Target::POSIX::Process

namespace ds2 {
namespace Target {
namespace POSIX {

namespace {

template <typename AUXV>
inline void EnumerateELFAuxiliaryVector(
    std::string const &auxv,
    std::function<void(ELFSupport::AuxiliaryVectorEntry const &)> const &cb) {
  size_t count = auxv.size() / sizeof(AUXV);
  AUXV const *auxve = reinterpret_cast<AUXV const *>(&auxv[0]);

  for (size_t n = 0; n < count; n++) {
    ELFSupport::AuxiliaryVectorEntry entry = {(uint64_t)auxve[n].a_type,
                                              (uint64_t)auxve[n].a_un.a_val};
    cb(entry);
  }
}

template <typename EHDR, typename PHDR, typename DYNSYM>
inline ErrorCode GetELFSharedLibraryInfoAddress(ELFProcess *process,
                                                Address &address) {
  EHDR eh;
  PHDR ph;
  DYNSYM dynsym;

  // 1. Find the PHDR, number of entries, size of each entry, and the loadBase
  //    of the binary.
  //    Assumption: PHDR is attached to the ELF header. This isn't enforced but
  //    it almost always occurs in practice.
  uint64_t phdrAddress = process->getAuxiliaryVectorValue(AT_PHDR);
  size_t phdrEntrySize = process->getAuxiliaryVectorValue(AT_PHENT);
  size_t phdrNumEntry = process->getAuxiliaryVectorValue(AT_PHNUM);
  uint64_t loadBase = phdrAddress - sizeof(eh);
  if (phdrAddress == 0 || phdrEntrySize == 0 || phdrNumEntry == 0) {
    DS2LOG(Debug, "Unable to locate PHDR info, "
                  "Dynamic Library info will not be available.");
    return kErrorUnsupported;
  }

  // 2. Find PT_DYNAMIC (.dynamic) program header and what the ELF believes is
  //    the loadBase. Find the size of the dynamic section as well.
  uint64_t elfLoadBase = 0;
  bool foundElfLoadBase = false;
  uint64_t dotDynamicAddress = 0;
  bool foundDotDynamicAddress = false;
  uint64_t dotDynamicSize = 0;
  for (size_t n = 0; n < phdrNumEntry; n++) {
    CHK(process->readMemory(phdrAddress + n * phdrEntrySize, &ph, sizeof(ph)));

    if ((!foundElfLoadBase || ph.p_vaddr < elfLoadBase) &&
        ph.p_type == PT_LOAD) {
      elfLoadBase = ph.p_vaddr;
      foundElfLoadBase = true;
    }

    if (ph.p_type == PT_DYNAMIC) {
      dotDynamicAddress = ph.p_vaddr;
      dotDynamicSize = ph.p_memsz;
      foundDotDynamicAddress = true;
    }
  }

  if (!foundDotDynamicAddress || !foundElfLoadBase) {
    DS2LOG(Debug, "Unable to locate PT_DYNAMIC or any PT_LOAD, "
                  "Dynamic Library info will not be available.");
    return kErrorUnsupported;
  }

  // 3. Find DT_DEBUG in .dynamic section
  dotDynamicAddress += (loadBase - elfLoadBase);
  size_t entriesCount = dotDynamicSize / sizeof(dynsym);
  for (size_t n = 0; n < entriesCount; n++) {
    CHK(process->readMemory(dotDynamicAddress + n * sizeof(dynsym), &dynsym,
                            sizeof(dynsym)));

    // 4. Return &DT_DEBUG->d_ptr, which is filled by the dynamic linker
    //    pointing to the link_map.
    if (dynsym.d_tag == DT_DEBUG) {
      address = (dotDynamicAddress + n * sizeof(dynsym)) + sizeof(dynsym.d_tag);
      return kSuccess;
    }
  }

  DS2LOG(Debug, "DT_DEBUG was not found in .dynamic, "
                "Dynamic Library info will not be available.");
  return kErrorUnsupported;
}

//
// At the address returned by getSharedLibraryInfoAddress
// we find an r_debug struct. Unfortunately we may be targetting
// a 32-bit process from 64-bit, so we need to add some helping
// code since the link.h header does not provide differentiated
// versions for 32 and 64bits.
//

//
// The r_debug struct is:
//
//   int r_version
//   struct link_map *r_map;
//   ElfW_Addr r_brk;
//   int r_state;
//   ElfW_Addr r_ldbase;
//
// The link_map struct is:
//
//   ElfW_Addr l_addr;
//   char *l_name;
//   ElfW_Dyn *l_ld;
//   struct link_map *l_next;
//   struct link_map *l_prev;
//
template <typename T> struct ELFDebug {
  int version;
  T mapAddress;
  T brk;
  int state;
  T ldbase;
};

template <typename T> struct ELFLinkMap {
  T baseAddress;
  T nameAddress;
  T ldAddress;
  T nextAddress;
  T prevAddress;
};

template <typename T>
inline ErrorCode ReadELFDebug(ELFProcess *process, Address address,
                              ELFDebug<T> &debug) {
  return process->readMemory(address, &debug, sizeof(debug));
}

template <typename T>
inline ErrorCode ReadELFLinkMap(ELFProcess *process, Address address,
                                ELFLinkMap<T> &linkMap) {
  return process->readMemory(address, &linkMap, sizeof(linkMap));
}

template <typename T>
ErrorCode
EnumerateLinkMap(ELFProcess *process, Address addressToDPtr,
                 std::function<void(SharedLibraryInfo const &)> const &cb) {
  ELFDebug<T> debug;
  ELFLinkMap<T> linkMap;
  T address;
  T linkMapAddress;

  CHK(process->readMemory(addressToDPtr, &address, sizeof(address)));

  // If the address is 0, it means the dynamic linker hasn't filled
  // DT_DEBUG->d_ptr and the link map is not available yet.
  if (address == 0) {
    return kErrorBusy;
  }

  CHK(ReadELFDebug(process, address, debug));

// Android and FreeBSD don't have a definition for LAV_CURRENT so we skip this
// check.
#if defined(LAV_CURRENT)
  if (debug.version != LAV_CURRENT) {
    return kErrorUnsupported;
  }
#endif

  linkMapAddress = debug.mapAddress;
  while (linkMapAddress != 0) {
    SharedLibraryInfo shlib;

    CHK(ReadELFLinkMap(process, linkMapAddress, linkMap));
    CHK(process->readString(linkMap.nameAddress, shlib.path, PATH_MAX));

    shlib.svr4.mapAddress = linkMapAddress;
    shlib.svr4.baseAddress = linkMap.baseAddress;
    shlib.svr4.ldAddress = linkMap.ldAddress;
    shlib.sections.clear();

#if defined(OS_LINUX) && !defined(PLATFORM_ANDROID)
    // On non-android linux systems, main executable has an empty path.
    shlib.main = shlib.path.empty();
#elif defined(OS_LINUX) && defined(PLATFORM_ANDROID)
    // On android, the main executable has a load address of 0.
    shlib.main = shlib.svr4.ldAddress == 0;
#elif defined(OS_FREEBSD)
    // FIXME(sas): not sure how exactly to determine this on FreeBSD.
    shlib.main = false;
#else
#error "Target not supported."
#endif

    cb(shlib);

    linkMapAddress = linkMap.nextAddress;
  }

  return kSuccess;
}
} // namespace

ErrorCode ELFProcess::getAuxiliaryVector(std::string &auxv) {
  ErrorCode error = updateAuxiliaryVector();
  if (error == kSuccess || error == kErrorAlreadyExist) {
    auxv = _auxiliaryVector;
    error = kSuccess;
  }
  return error;
}

uint64_t ELFProcess::getAuxiliaryVectorValue(uint64_t type) {
  uint64_t value = 0;
  bool found = false;

  enumerateAuxiliaryVector([&](ELFSupport::AuxiliaryVectorEntry const &entry) {
    if (found)
      return;

    if (entry.type == type) {
      value = entry.value;
      found = true;
    }
  });

  return value;
}

//
// Inheriting classes should override and call this method
// to complete the process information; if this method
// returns kErrorAlreadyExist then the information is
// already present and the call should be considered
// successful.
//
ErrorCode ELFProcess::updateInfo() {
  if (_info.pid == _pid) {
    return kErrorAlreadyExist;
  }

  // This is tricky, we don't know the load base, and to
  // do so we need to update the auxiliary vector, but
  // in order to interpret it we need to determine first
  // if it's 32 or 64bits. In this point of the code
  // we don't have idea of what is our target platform,
  // so we'll do an empirical test.
  if (!_loadBase.valid() || !_entryPoint.valid()) {
    ErrorCode error = updateAuxiliaryVector();
    if (error != kSuccess && error != kErrorAlreadyExist) {
      return error;
    }

    // Hack to use enumerateAuxiliaryVector before information is set.
    _info.pid = _pid;

    bool possibly32 = false;
    for (size_t n = 0; n < _auxiliaryVector.size() / (sizeof(uint64_t) * 2);
         n += 2) {
      if (*(uint64_t *)&_auxiliaryVector[n * sizeof(uint64_t) * 2] >
          std::numeric_limits<uint32_t>::max()) {
        possibly32 = true;
      }
    }

    if (!possibly32) {
      _info.cpuType = kCPUTypeAll64;
    } else {
      _info.cpuType = kCPUTypeAll;
    }

    _entryPoint = getAuxiliaryVectorValue(AT_ENTRY);

    // Query the memory region information to know where
    // is the load base.
    MemoryRegionInfo mri;
#if defined(OS_LINUX)
    MemoryRegionInfo prevMri;
#endif
    error = getMemoryRegionInfo(_entryPoint, mri);
    if (error != kSuccess) {
      goto mri_error;
    }

#if defined(OS_LINUX)
    // Here, we use the interpreter's base address instead of the binary's. The
    // interpreter has an ELF header just the same as the binary, so we can get
    // most of the necessary info from it.
    // If this fails for any reason, an easy fallback is to grab the PHDR and
    // subtract the size of the ELF header. This should be good enough, though.
    _loadBase = getAuxiliaryVectorValue(AT_BASE);
#else
    _loadBase = mri.start;
#endif

  mri_error:
    // Restore the hack.
    _info.pid = kAnyProcessId;
    _info.cpuType = kCPUTypeAny;

    return error;
  }

  //
  // Read the ELF header to determine machine type.
  //
  union {
    Elf32_Ehdr e32;
    Elf64_Ehdr e64;
  } eh;

  CHK(readMemory(loadBase(), &eh, sizeof(eh)));

  if (eh.e32.e_ident[EI_MAG0] != ELFMAG0 ||
      eh.e32.e_ident[EI_MAG1] != ELFMAG1 ||
      eh.e32.e_ident[EI_MAG2] != ELFMAG2 || eh.e32.e_ident[EI_MAG3] != ELFMAG3)
    return kErrorUnsupported;

  //
  // Obtain the ELF machine type, which is the native CPU type.
  //
  uint32_t dataEndian = ELFDATANONE;
  uint32_t machineType = kInvalidCPUType;
  bool is64Bit = false;

  if (eh.e32.e_ident[EI_CLASS] == ELFCLASS32) {
    machineType = eh.e32.e_machine;
    dataEndian = eh.e32.e_ident[EI_DATA];
    is64Bit = false;
  } else if (eh.e64.e_ident[EI_CLASS] == ELFCLASS64) {
    machineType = eh.e64.e_machine;
    dataEndian = eh.e64.e_ident[EI_DATA];
    is64Bit = true;
  }

  //
  // If we have invalid data then fail.
  //
  if (machineType == kInvalidCPUType ||
      (dataEndian != ELFDATA2LSB && dataEndian != ELFDATA2MSB))
    return kErrorUnsupported;

  //
  // Now obtain the DebugServer CPU type out of the ELF machine type,
  // we also validate that this ELF file is really for the current
  // architecture.
  //
  if (!ELFSupport::MachineTypeToCPUType(machineType, is64Bit, _info.cpuType,
                                        _info.cpuSubType))
    return kErrorUnsupported;

  //
  // Save the native CPU type.
  //
  _info.nativeCPUType = machineType;
  _info.nativeCPUSubType = kInvalidCPUType;

  //
  // Now continue setting up other fields.
  //
  _info.pid = _pid;
  _info.endian = (dataEndian == ELFDATA2MSB) ? kEndianBig : kEndianLittle;
  _info.pointerSize = is64Bit ? 8 : 4;

  //
  // Enumerate the Auxiliary Vector to extract UID, EUID, GID and EGID;
  // we must do this _after_ setting the _info.pid because we'll
  // be called by enumerateAuxiliaryVector!
  //
  enumerateAuxiliaryVector([&](ELFSupport::AuxiliaryVectorEntry const &entry) {
    switch (entry.type) {
    case AT_GID:
      _info.realGid = entry.value;
      break;
    case AT_EGID:
      _info.effectiveGid = entry.value;
      break;
    case AT_UID:
      _info.realUid = entry.value;
      break;
    case AT_EUID:
      _info.effectiveUid = entry.value;
      break;
    }
  });

  //
  // It's up to the inheriting class filling remaining fields
  // (parentPid and archFlags)
  //

  return kSuccess;
}

//
// Inheriting class should call this method and then
// read data into _auxiliaryVector buffer; if this method
// returns kErrorAlreadyExist then the information is
// already present and the call should be considered
// successful, any other error should be ignored.
//
ErrorCode ELFProcess::updateAuxiliaryVector() {
#if 0
    if (!_auxiliaryVector.empty())
        return kErrorAlreadyExist;
#endif

  return kSuccess;
}

//
// Enumerate entries in the ELF auxiliary vector.
//
ErrorCode ELFProcess::enumerateAuxiliaryVector(
    std::function<void(ELFSupport::AuxiliaryVectorEntry const &)> const &cb) {
  //
  // We need to know how big is our ELF
  // and be sure the auxiliary vector has
  // been read in memory.
  //
  ErrorCode error;

  error = updateAuxiliaryVector();
  if (error != kSuccess && error != kErrorAlreadyExist) {
    return error;
  }

  if (_auxiliaryVector.empty()) {
    return kSuccess;
  }

  if (CPUTypeIs64Bit(_info.cpuType)) {
    EnumerateELFAuxiliaryVector<Elf64_auxv_t>(_auxiliaryVector, cb);
  } else {
    EnumerateELFAuxiliaryVector<Elf32_auxv_t>(_auxiliaryVector, cb);
  }

  return kSuccess;
}

//
// Retrieves the shared library info address pointer.
//
ErrorCode ELFProcess::getSharedLibraryInfoAddress(Address &address) {
  if (!_sharedLibraryInfoAddress.valid()) {
    //
    // Force the updating of process information.
    //
    ErrorCode error = updateAuxiliaryVector();
    if (error != kSuccess && error != kErrorAlreadyExist)
      return error;

    if (CPUTypeIs64Bit(_info.cpuType)) {
      error = GetELFSharedLibraryInfoAddress<Elf64_Ehdr, Elf64_Phdr, Elf64_Dyn>(
          this, _sharedLibraryInfoAddress);
    } else {
      error = GetELFSharedLibraryInfoAddress<Elf32_Ehdr, Elf32_Phdr, Elf32_Dyn>(
          this, _sharedLibraryInfoAddress);
    }

    if (error != kSuccess)
      return error;
  }

  address = _sharedLibraryInfoAddress;
  return kSuccess;
}

//
// Enumerates the linkmap of this SVR4 ELF process.
//
ErrorCode ELFProcess::enumerateSharedLibraries(
    std::function<void(SharedLibraryInfo const &)> const &cb) {
  Address address;
  CHK(getSharedLibraryInfoAddress(address));

  if (CPUTypeIs64Bit(_info.cpuType)) {
    return EnumerateLinkMap<uint64_t>(this, address, cb);
  } else {
    return EnumerateLinkMap<uint32_t>(this, address, cb);
  }
}
} // namespace POSIX
} // namespace Target
} // namespace ds2
