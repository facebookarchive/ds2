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

  ErrorCode error = process->readMemory(process->loadBase(), &eh, sizeof(eh));
  if (error != ds2::kSuccess)
    return error;

  if (eh.e_phnum == 0)
    return ds2::kErrorUnsupported;

  //
  // 1. Find PT_DYNAMIC (.dynamic) program header, and the
  //    expected load base of this ELF.
  //
  uint64_t elfLoadBase = 0;
  bool foundElfLoadBase = false;
  uint64_t phdrAddress = process->loadBase() + eh.e_phoff;
  for (size_t n = 0; n < eh.e_phnum; n++) {
    error = process->readMemory(phdrAddress, &ph, sizeof(ph));
    if (error != ds2::kSuccess)
      return error;

    if (!foundElfLoadBase && ph.p_type == PT_LOAD) {
      elfLoadBase = ph.p_paddr;
      foundElfLoadBase = true;
    }

    if (ph.p_type == PT_DYNAMIC)
      break;

    phdrAddress += eh.e_phentsize;
  }

  if (ph.p_type != PT_DYNAMIC)
    return ds2::kErrorUnsupported;

  //
  // 2. Find DT_DEBUG in .dynamic section
  //
  uint64_t dynsymAddress = process->loadBase() + ph.p_paddr - elfLoadBase;
  size_t entriesCount = ph.p_memsz / sizeof(dynsym);
  for (size_t n = 0; n < entriesCount; n++) {
    error = process->readMemory(dynsymAddress, &dynsym, sizeof(dynsym));
    if (error != ds2::kSuccess)
      return error;

    //
    // 3. Return &DT_DEBUG->d_ptr, which is filled by the dynamic linker
    //    pointing to the link_map.
    //
    if (dynsym.d_tag == DT_DEBUG) {
      address = dynsymAddress + sizeof(dynsym.d_tag);
      return ds2::kSuccess;
    }

    dynsymAddress += sizeof(dynsym);
  }

  return ds2::kErrorUnsupported;
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

  ErrorCode error =
      process->readMemory(addressToDPtr, &address, sizeof(address));
  if (error != ds2::kSuccess)
    return error;

  // If the address is 0, it means the dynamic linker hasn't filled
  // DT_DEBUG->d_ptr and the link map is not available yet.
  if (address == 0) {
    return kErrorBusy;
  }

  error = ReadELFDebug(process, address, debug);
  if (error != ds2::kSuccess)
    return error;

// Android doesn't have a definition for LAV_CURRENT, so we skip this check.
#if !defined(PLATFORM_ANDROID) && !defined(OS_FREEBSD)
  if (debug.version != LAV_CURRENT)
    return ds2::kErrorUnsupported;
#endif

  linkMapAddress = debug.mapAddress;
  while (linkMapAddress != 0) {
    error = ReadELFLinkMap(process, linkMapAddress, linkMap);
    if (error != ds2::kSuccess)
      return error;

    SharedLibraryInfo shlib;

    error = process->readString(linkMap.nameAddress, shlib.path, PATH_MAX);
    if (error != ds2::kSuccess)
      return error;

    shlib.svr4.mapAddress = linkMapAddress;
    shlib.svr4.baseAddress = linkMap.baseAddress;
    shlib.svr4.ldAddress = linkMap.ldAddress;
    shlib.sections.clear();

#if defined(OS_LINUX) && !defined(PLATFORM_ANDROID)
    // On non-android linux systems, main executable has an empty path.
    shlib.main = shlib.path.empty();
#elif defined(OS_LINUX) && defined(PLATFORM_ANDROID)
    // On android, libraries have just their basename as path, except for
    // /system/bin/linker and /system/bin/linker64 that have the full path.
    // The main executable also has a full path, except when debugging an app,
    // where the app's bundle indentifier is used as path. (wat?)
    shlib.main = shlib.svr4.ldAddress == 0 ||
                 (shlib.path.find("/system/bin/linker") != 0 &&
                  !shlib.path.empty() && shlib.path[0] == '/');
#elif defined(OS_FREEBSD)
    // FIXME(sas): not sure how exactly to determine this on FreeBSD.
    shlib.main = false;
#else
#error "Target not supported."
#endif

    cb(shlib);

    linkMapAddress = linkMap.nextAddress;
  }

  return ds2::kSuccess;
}
}

//
// This is a SVR4 ELF process, we want this method because GDB
// distinguishes between SVR4 and non-SVR4 processes to read
// libraries information.
//
bool ELFProcess::isELFProcess() const { return true; }

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
  if (_info.pid == _pid)
    return kErrorAlreadyExist;

  ErrorCode error;

  // This is tricky, we don't know the load base, and to
  // do so we need to update the auxiliary vector, but
  // in order to interpret it we need to determine first
  // if it's 32 or 64bits. In this point of the code
  // we don't have idea of what is our target platform,
  // so we'll do an empirical test.
  if (!_loadBase.valid() || !_entryPoint.valid()) {
    error = updateAuxiliaryVector();
    if (error != kSuccess && error != kErrorAlreadyExist)
      return error;

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
    if (error != kSuccess)
      goto mri_error;

#if defined(OS_LINUX)
    // For some reason, on Android 5.1 and up some ELF segments get loaded in
    // memory multiple smaller contiguous regions instead of being loaded as a
    // single big mapping.
    // This means we can't properly identify the beginning of the segment by
    // just looking at the start address of the mapping; we have to inspect
    // contiguous mappings instead and see if they are contiguous in the input
    // file and then take the start address of the first mapping of the segment
    // we're inspecting.
    do {
      prevMri = mri;
      error = getMemoryRegionInfo(mri.start - 1, mri);
      if (error != kSuccess)
        goto mri_error;
    } while (mri.backingFile == prevMri.backingFile &&
             mri.backingFileInode == prevMri.backingFileInode &&
             mri.protection == prevMri.protection &&
             mri.backingFileOffset + mri.length == prevMri.backingFileOffset);

    _loadBase = prevMri.start;
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

  error = readMemory(loadBase(), &eh, sizeof(eh));
  if (error != kSuccess)
    return error;

  if (eh.e32.e_ident[EI_MAG0] != ELFMAG0 ||
      eh.e32.e_ident[EI_MAG1] != ELFMAG1 ||
      eh.e32.e_ident[EI_MAG2] != ELFMAG2 || eh.e32.e_ident[EI_MAG3] != ELFMAG3)
    return ds2::kErrorUnsupported;

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
    return ds2::kErrorUnsupported;

  //
  // Now obtain the DebugServer CPU type out of the ELF machine type,
  // we also validate that this ELF file is really for the current
  // architecture.
  //
  if (!ELFSupport::MachineTypeToCPUType(machineType, is64Bit, _info.cpuType,
                                        _info.cpuSubType))
    return ds2::kErrorUnsupported;

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
  if (error != kSuccess && error != kErrorAlreadyExist)
    return error;

  if (_auxiliaryVector.empty())
    return kSuccess;

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
    _info.pid = -1;

    ErrorCode error = updateInfo();
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
  ErrorCode error = getSharedLibraryInfoAddress(address);
  if (error != kSuccess)
    return error;

  if (CPUTypeIs64Bit(_info.cpuType)) {
    return EnumerateLinkMap<uint64_t>(this, address, cb);
  } else {
    return EnumerateLinkMap<uint32_t>(this, address, cb);
  }
}
}
}
}
