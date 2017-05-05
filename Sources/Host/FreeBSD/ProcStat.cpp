//
// Copyright (c) 2015, Jakub Klama <jakub@ixsystems.com
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

// clang-format off
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <libprocstat.h>
// clang-format on
#include <libutil.h>
#include <string>
#include <sys/elf.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/user.h>

#include "DebugServer2/Host/FreeBSD/ProcStat.h"
#include "DebugServer2/Support/POSIX/ELFSupport.h"

namespace ds2 {
namespace Host {
namespace FreeBSD {

using ds2::Support::ELFSupport;

bool ProcStat::GetProcessInfo(ProcessId pid, ProcessInfo &info) {
  struct kinfo_proc *kip;

  kip = kinfo_getproc(pid);
  if (kip == nullptr)
    return false;

  info.pid = pid;
  info.parentPid = kip->ki_ppid;
  info.realUid = kip->ki_ruid;
  info.effectiveUid = kip->ki_uid;
  info.realGid = kip->ki_rgid;
  info.effectiveGid = kip->ki_svgid;
  info.name = std::string(kip->ki_comm);

  return true;
}

bool ProcStat::GetThreadState(pid_t pid, pid_t tid, int &state, int &cpu) {
  struct procstat *pstat;
  struct kinfo_proc *kip;
  unsigned int count, i;

  pstat = procstat_open_sysctl();
  kip = procstat_getprocs(pstat, KERN_PROC_PID | KERN_PROC_INC_THREAD, pid,
                          &count);

  if (kip == NULL) {
    state = kProcStateDead;
    cpu = 0;
    return true;
  }

  for (i = 0; i < count; i++) {
    if (kip[i].ki_tid == tid || kip[i].ki_pid == tid) {
      state = kip[i].ki_stat;
      cpu = kip[i].ki_lastcpu;
      procstat_freeprocs(pstat, kip);
      procstat_close(pstat);
      return true;
    }
  }

  procstat_freeprocs(pstat, kip);
  procstat_close(pstat);
  return false;
}

bool ProcStat::GetProcessMap(pid_t pid,
                             std::vector<ds2::MemoryRegionInfo> &entries) {
  struct procstat *pstat;
  struct kinfo_proc *kip;
  struct kinfo_vmentry *vmp;
  unsigned int count, i;

  pstat = procstat_open_sysctl();
  kip = kinfo_getproc(pid);
  if (kip == nullptr)
    return false;
  vmp = procstat_getvmmap(pstat, kip, &count);

  for (i = 0; i < count; i++) {
    ds2::MemoryRegionInfo region;
    region.start = vmp[i].kve_start;
    region.length = vmp[i].kve_end - vmp[i].kve_start;

    if (vmp[i].kve_protection & KVME_PROT_READ)
      region.protection |= ds2::kProtectionRead;

    if (vmp[i].kve_protection & KVME_PROT_WRITE)
      region.protection |= ds2::kProtectionWrite;

    if (vmp[i].kve_protection & KVME_PROT_EXEC)
      region.protection |= ds2::kProtectionExecute;

    entries.push_back(region);
  }

  procstat_freeprocs(pstat, kip);
  procstat_close(pstat);
  return true;
}

bool ProcStat::EnumerateAuxiliaryVector(
    pid_t pid,
    std::function<void(ELFSupport::AuxiliaryVectorEntry const &)> const &cb) {

  struct procstat *pstat;
  struct kinfo_proc *kip;
  Elf_Auxinfo *auxvp;
  unsigned int count, i;

  pstat = procstat_open_sysctl();
  kip = kinfo_getproc(pid);
  if (kip == nullptr)
    return false;
  auxvp = procstat_getauxv(pstat, kip, &count);

  for (i = 0; i < count; i++) {
    ELFSupport::AuxiliaryVectorEntry entry;
    entry.type = auxvp[i].a_type;
    entry.value = auxvp[i].a_un.a_val;
    cb(entry);
  }

  procstat_freeauxv(pstat, auxvp);
  procstat_close(pstat);
  return true;
}

void ProcStat::EnumerateProcesses(
    bool allUsers, UserId const &uid,
    std::function<void(pid_t pid, uid_t uid)> const &cb) {
  struct procstat *pstat;
  struct kinfo_proc *kip;
  unsigned int count, i;

  pstat = procstat_open_sysctl();
  kip = procstat_getprocs(pstat, KERN_PROC_PROC, 0, &count);

  for (i = 0; i < count; i++) {
    cb(kip[i].ki_pid, kip[i].ki_uid);
  }

  procstat_freeprocs(pstat, kip);
  procstat_close(pstat);
}

void ProcStat::EnumerateThreads(pid_t pid,
                                std::function<void(pid_t tid)> const &cb) {
  struct procstat *pstat;
  struct kinfo_proc *kip;
  unsigned int count, i;

  pstat = procstat_open_sysctl();
  kip = procstat_getprocs(pstat, KERN_PROC_PID | KERN_PROC_INC_THREAD, pid,
                          &count);

  for (i = 0; i < count; i++)
    cb(kip[i].ki_tid);

  procstat_freeprocs(pstat, kip);
  procstat_close(pstat);
}

std::string ProcStat::GetThreadName(ProcessId pid, ThreadId tid) {
  struct procstat *pstat;
  struct kinfo_proc *kip;
  std::string result;
  unsigned int count, i;

  pstat = procstat_open_sysctl();
  kip = procstat_getprocs(pstat, KERN_PROC_PID | KERN_PROC_INC_THREAD, pid,
                          &count);

  for (i = 0; i < count; i++) {
    if (kip[i].ki_tid == tid) {
      result = kip[i].ki_tdname;
      procstat_freeprocs(pstat, kip);
      procstat_close(pstat);
      return result;
    }
  }

  procstat_freeprocs(pstat, kip);
  procstat_close(pstat);
  return "<unknown>";
}

std::string ProcStat::GetExecutablePath(ProcessId pid) { return "unknown"; }
} // namespace FreeBSD
} // namespace Host
} // namespace ds2
