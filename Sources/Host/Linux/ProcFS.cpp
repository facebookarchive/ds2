//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/Linux/ProcFS.h"
#include "DebugServer2/Host/Linux/ExtraWrappers.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Support/POSIX/ELFSupport.h"
#include "DebugServer2/Utils/HexValues.h"
#include "DebugServer2/Utils/Log.h"
#include "DebugServer2/Utils/String.h"

#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <elf.h>
#include <libgen.h>

using ds2::CPUType;
using ds2::Support::ELFSupport;

namespace ds2 {
namespace Host {
namespace Linux {

//
// The fields of the /proc/uptime file.
//
#define UPTIME_F_RUN_TIME 0
#define UPTIME_F_IDLE_TIME 1

//
// The fields of the /proc/XYZ/stat file.
//
#define STAT_F_PID 0
#define STAT_F_TCOMM 1
#define STAT_F_STATE 2
#define STAT_F_PPID 3
#define STAT_F_PGRP 4
#define STAT_F_SID 5
#define STAT_F_TTY_NR 6
#define STAT_F_TTY_PGRP 7
#define STAT_F_FLAGS 8
#define STAT_F_MIN_FLT 9
#define STAT_F_CMIN_FLT 10
#define STAT_F_MAJ_FLT 11
#define STAT_F_CMAJ_FLT 12
#define STAT_F_UTIME 13
#define STAT_F_STIME 14
#define STAT_F_CUTIME 15
#define STAT_F_CSTIME 16
#define STAT_F_PRIORITY 17
#define STAT_F_NICE 18
#define STAT_F_NUM_THREADS 19
#define STAT_F_IT_REAL_VALUE 20
#define STAT_F_START_TIME 21
#define STAT_F_VSIZE 22
#define STAT_F_RSS 23
#define STAT_F_RSSLIM 24
#define STAT_F_START_CODE 25
#define STAT_F_END_CODE 26
#define STAT_F_START_STACK 27
#define STAT_F_ESP 28
#define STAT_F_EIP 29
#define STAT_F_PENDING 30
#define STAT_F_BLOCKED 31
#define STAT_F_SIGIGN 32
#define STAT_F_SIGCATCH 33
#define STAT_F_WCHAN 34
#define STAT_F_EXIT_SIGNAL 37
#define STAT_F_TASK_CPU 38
#define STAT_F_RT_PRIORITY 39
#define STAT_F_POLICY 40
#define STAT_F_BLKIO_TICKS 41
#define STAT_F_GTIME 42
#define STAT_F_CGTIME 43
#define STAT_F_START_DATA 44
#define STAT_F_END_DATA 45
#define STAT_F_START_BRK 46

static void MakePath(char *path, size_t maxsize, pid_t pid, char const *what) {
  if (pid < 0) {
    pid = -pid;
  }

  if (pid == 0) {
    if (what == nullptr) {
      ds2::Utils::SNPrintf(path, maxsize, "/proc/self");
    } else {
      ds2::Utils::SNPrintf(path, maxsize, "/proc/self/%s", what);
    }
  } else {
    if (what == nullptr) {
      ds2::Utils::SNPrintf(path, maxsize, "/proc/%d", pid);
    } else {
      ds2::Utils::SNPrintf(path, maxsize, "/proc/%d/%s", pid, what);
    }
  }
}

static void MakePath(char *path, size_t maxsize, pid_t pid, pid_t tid,
                     char const *what) {
  if (pid < 0) {
    pid = -pid;
  }

  if (tid < 0) {
    tid = -tid;
  }

  if (pid == tid) {
    MakePath(path, maxsize, pid, what);
    return;
  }

  if (pid == 0) {
    if (tid == 0) {
      tid = gettid();
    }

    if (what == nullptr) {
      ds2::Utils::SNPrintf(path, maxsize, "/proc/self/task/%d", tid);
    } else {
      ds2::Utils::SNPrintf(path, maxsize, "/proc/self/task/%d/%s", tid, what);
    }
  } else {
    if (tid == 0) {
      tid = pid;
    }

    if (what == nullptr) {
      ds2::Utils::SNPrintf(path, maxsize, "/proc/%d/task/%d", pid, tid);
    } else {
      ds2::Utils::SNPrintf(path, maxsize, "/proc/%d/task/%d/%s", pid, tid,
                           what);
    }
  }
}

int ProcFS::OpenFd(char const *what, int mode) {
  char path[PATH_MAX + 1];
  ds2::Utils::SNPrintf(path, PATH_MAX, "/proc/%s", what);
  return open(path, mode);
}

int ProcFS::OpenFd(pid_t pid, char const *what, int mode) {
  return OpenFd(pid, pid, what, mode);
}

int ProcFS::OpenFd(pid_t pid, pid_t tid, char const *what, int mode) {
  char path[PATH_MAX + 1];
  MakePath(path, PATH_MAX, pid, tid, what);
  return open(path, mode);
}

FILE *ProcFS::OpenFILE(char const *what, char const *mode) {
  char path[PATH_MAX + 1];
  ds2::Utils::SNPrintf(path, PATH_MAX, "/proc/%s", what);
  FILE *res = fopen(path, mode);
  if (res == nullptr)
    DS2LOG(Error, "can't open %s: %s", path, strerror(errno));
  return res;
}

FILE *ProcFS::OpenFILE(pid_t pid, char const *what, const char *mode) {
  return OpenFILE(pid, pid, what, mode);
}

FILE *ProcFS::OpenFILE(pid_t pid, pid_t tid, char const *what,
                       char const *mode) {
  char path[PATH_MAX + 1];
  MakePath(path, PATH_MAX, pid, tid, what);
  FILE *res = fopen(path, mode);
  if (res == nullptr)
    DS2LOG(Error, "can't open %s: %s", path, strerror(errno));
  return res;
}

DIR *ProcFS::OpenDIR(char const *what) {
  char path[PATH_MAX + 1];
  ds2::Utils::SNPrintf(path, PATH_MAX, "/proc/%s", what);
  return opendir(path);
}

DIR *ProcFS::OpenDIR(pid_t pid, char const *what) {
  return OpenDIR(pid, pid, what);
}

DIR *ProcFS::OpenDIR(pid_t pid, pid_t tid, char const *what) {
  char path[PATH_MAX + 1];
  MakePath(path, PATH_MAX, pid, tid, what);
  return opendir(path);
}

bool ProcFS::ReadLink(pid_t pid, char const *what, char *buf, size_t bufsiz) {
  return ReadLink(pid, pid, what, buf, bufsiz);
}

bool ProcFS::ReadLink(pid_t pid, pid_t tid, char const *what, char *buf,
                      size_t bufsiz) {
  char path[PATH_MAX + 1];
  MakePath(path, PATH_MAX, pid, tid, what);
  return readlink(path, buf, bufsiz) == 0;
}

void ProcFS::ParseKeyValue(
    FILE *fp, size_t maxsize, char sep,
    std::function<bool(char const *, char const *)> const &cb) {
  char line[maxsize + 1];

  rewind(fp);

  for (;;) {
    char *ep, *lp = fgets(line, maxsize, fp);
    if (lp == nullptr)
      break;

    ep = lp + strlen(lp) - 1;
    while (ep != lp && isspace(*ep)) {
      ep--;
    }
    *++ep = '\0';

    if (*lp == '\0')
      continue;

    char *key = lp;
    while (isspace(*key)) {
      key++;
    }

    char *value = strchr(key, sep);
    if (value == nullptr) {
      value = key;
      key = nullptr;
    } else {
      *value++ = '\0';
    }

    while (isspace(*value)) {
      value++;
    }

    if (!cb(key, value))
      break;
  }
}

void ProcFS::ParseValues(FILE *fp, size_t maxsize, char sep, bool includeSep,
                         std::function<bool(size_t, char const *)> const &cb) {
  char line[maxsize + 1];

  rewind(fp);

  for (;;) {
    char *ep, *lp = fgets(line, maxsize, fp);
    if (lp == nullptr)
      break;

    ep = lp + strlen(lp);
    for (size_t index = 0; lp < ep; index++) {
      char *value = lp;
      while (*lp != '\0' && *lp != sep) {
        lp++;
      }

      while (*lp != '\0' && *(lp + 1) == sep) {
        lp++;
      }

      char swap;
      if (!includeSep) {
        *lp++ = '\0';
      } else {
        swap = *++lp;
        *lp = '\0';
      }

      if (!cb(index, value))
        break;

      if (includeSep) {
        *lp = swap;
      }
    }
  }
}

bool ProcFS::ReadStat(pid_t pid, Stat &stat) {
  return ReadStat(pid, pid, stat);
}

bool ProcFS::ReadUptime(Uptime &uptime) {
  FILE *fp = OpenFILE("uptime");
  if (fp == nullptr)
    return false;

  ParseValues(
      fp, 1024, ' ', false, [&](size_t index, char const *value) -> bool {
        char *end;

        switch (index) {
        case UPTIME_F_RUN_TIME:
          uptime.run_time.tv_sec = strtoll(value, &end, 0);
          if (*end++ == '.') {
            uptime.run_time.tv_nsec = strtoll(end, nullptr, 0) * 10000000;
          }
          break;

        case UPTIME_F_IDLE_TIME:
          uptime.idle_time.tv_sec = strtoll(value, &end, 0);
          if (*end++ == '.') {
            uptime.idle_time.tv_nsec = strtoll(end, nullptr, 0) * 10000000;
          }
          break;
        }

        return true;
      });

  std::fclose(fp);
  return true;
}

bool ProcFS::ReadStat(pid_t pid, pid_t tid, Stat &stat) {
  FILE *fp = OpenFILE(pid, tid, "stat");
  if (fp == nullptr)
    return false;

  // The complicated logic about the comm field is brought to you by the insane
  // way Linux "encodes" that field.  An evil executable might contain ')' in
  // its name and break the parsing.
  std::string comm;
  size_t field_index = STAT_F_PID;
  bool may_end_comm = false;

  ParseValues(fp, 1024, ' ', true, [&](size_t, char const *value) -> bool {
    if (field_index == STAT_F_TCOMM) {
      //
      // Is this really the end?
      //
      if (may_end_comm && value[strlen(value) - 2] != ')') {
        //
        // Yes, save the "tcomm" field, and advance field index.
        //
        comm = comm.substr(0, comm.rfind(')'));
        strncpy(stat.tcomm, comm.c_str(), sizeof(stat.tcomm));
        field_index++;
      } else {
        if (!may_end_comm) {
          value++; // strip the initial '('
        }
        comm += value;
        //
        // Assume it is not the end.
        //
        may_end_comm = true;
        //
        // We'll process the next item to know if this was
        // really the end of the comm.
        //
        return true;
      }
    }

    switch (field_index++) {
    case STAT_F_STATE:
      stat.state = value[0];
      break;
    case STAT_F_PID:
      stat.pid = std::strtol(value, nullptr, 0);
      break;
    case STAT_F_PPID:
      stat.ppid = std::strtol(value, nullptr, 0);
      break;
    case STAT_F_PGRP:
      stat.pgrp = std::strtol(value, nullptr, 0);
      break;
    case STAT_F_SID:
      stat.sid = std::strtol(value, nullptr, 0);
      break;
    case STAT_F_TTY_NR:
      stat.tty_nr = std::strtoul(value, nullptr, 0);
      break;
    case STAT_F_TTY_PGRP:
      stat.tty_pgrp = std::strtol(value, nullptr, 0);
      break;
    case STAT_F_FLAGS:
      stat.flags = strtoull(value, nullptr, 0);
      break;
    case STAT_F_MIN_FLT:
      stat.min_flt = strtoull(value, nullptr, 0);
      break;
    case STAT_F_CMIN_FLT:
      stat.cmin_flt = strtoull(value, nullptr, 0);
      break;
    case STAT_F_MAJ_FLT:
      stat.maj_flt = strtoull(value, nullptr, 0);
      break;
    case STAT_F_CMAJ_FLT:
      stat.cmaj_flt = strtoull(value, nullptr, 0);
      break;
    case STAT_F_UTIME:
      stat.utime = strtoull(value, nullptr, 0);
      break;
    case STAT_F_STIME:
      stat.stime = strtoull(value, nullptr, 0);
      break;
    case STAT_F_CUTIME:
      stat.cutime = strtoull(value, nullptr, 0);
      break;
    case STAT_F_CSTIME:
      stat.cstime = strtoull(value, nullptr, 0);
      break;
    case STAT_F_PRIORITY:
      stat.priority = std::strtol(value, nullptr, 0);
      break;
    case STAT_F_NICE:
      stat.nice = std::strtol(value, nullptr, 0);
      break;
    case STAT_F_NUM_THREADS:
      stat.num_threads = std::strtoul(value, nullptr, 0);
      break;
    case STAT_F_IT_REAL_VALUE:
      stat.it_real_value = strtoull(value, nullptr, 0);
      break;
    case STAT_F_START_TIME:
      stat.start_time = strtoull(value, nullptr, 0);
      break;
    case STAT_F_VSIZE:
      stat.vsize = strtoull(value, nullptr, 0);
      break;
    case STAT_F_RSS:
      stat.rss = strtoull(value, nullptr, 0);
      break;
    case STAT_F_RSSLIM:
      stat.rsslim = strtoull(value, nullptr, 0);
      break;
    case STAT_F_START_CODE:
      stat.start_code = strtoull(value, nullptr, 0);
      break;
    case STAT_F_END_CODE:
      stat.end_code = strtoull(value, nullptr, 0);
      break;
    case STAT_F_START_STACK:
      stat.start_stack = strtoull(value, nullptr, 0);
      break;
    case STAT_F_ESP:
      stat.esp = strtoull(value, nullptr, 0);
      break;
    case STAT_F_EIP:
      stat.eip = strtoull(value, nullptr, 0);
      break;
    case STAT_F_PENDING:
      stat.pending = strtoull(value, nullptr, 0);
      break;
    case STAT_F_BLOCKED:
      stat.blocked = strtoull(value, nullptr, 0);
      break;
    case STAT_F_SIGIGN:
      stat.sigign = strtoull(value, nullptr, 0);
      break;
    case STAT_F_SIGCATCH:
      stat.sigcatch = strtoull(value, nullptr, 0);
      break;
    case STAT_F_WCHAN:
      stat.wchan = strtoull(value, nullptr, 0);
      break;
    case STAT_F_EXIT_SIGNAL:
      stat.exit_signal = std::strtoul(value, nullptr, 0);
      break;
    case STAT_F_TASK_CPU:
      stat.task_cpu = std::strtoul(value, nullptr, 0);
      break;
    case STAT_F_RT_PRIORITY:
      stat.rt_priority = std::strtol(value, nullptr, 0);
      break;
    case STAT_F_POLICY:
      stat.policy = std::strtoul(value, nullptr, 0);
      break;
    case STAT_F_BLKIO_TICKS:
      stat.blkio_ticks = strtoull(value, nullptr, 0);
      break;
    case STAT_F_GTIME:
      stat.gtime = strtoull(value, nullptr, 0);
      break;
    case STAT_F_CGTIME:
      stat.cgtime = strtoull(value, nullptr, 0);
      break;
    case STAT_F_START_DATA:
      stat.start_data = strtoull(value, nullptr, 0);
      break;
    case STAT_F_END_DATA:
      stat.end_data = strtoull(value, nullptr, 0);
      break;
    case STAT_F_START_BRK:
      stat.start_brk = strtoull(value, nullptr, 0);
      break;
    default:
      break;
    }
    return true;
  });

  std::fclose(fp);
  return true;
}

bool ProcFS::ReadProcessIds(pid_t pid, pid_t &ppid, uid_t &uid, uid_t &euid,
                            gid_t &gid, gid_t &egid) {
  FILE *fp = OpenFILE(pid, "status");
  if (fp == nullptr)
    return false;

  ParseKeyValue(fp, 1024, ':', [&](char const *key, char const *value) -> bool {
    char *eptr;
    if (strcmp(key, "PPid") == 0) {
      ppid = std::strtol(value, nullptr, 0);
    } else if (strcmp(key, "Uid") == 0) {
      // Real Effective
      uid = std::strtol(value, &eptr, 0);
      euid = std::strtol(eptr, &eptr, 0);
    } else if (strcmp(key, "Gid") == 0) {
      // Real Effective
      gid = std::strtol(value, &eptr, 0);
      egid = std::strtol(eptr, &eptr, 0);
      return false; // We're done, stop reading.
    }

    return true;
  });

  std::fclose(fp);
  return true;
}

pid_t ProcFS::GetProcessParentPid(pid_t pid) {
  FILE *fp = OpenFILE(pid, "status");
  if (fp == nullptr)
    return 0;

  pid_t ppid = 0;
  ParseKeyValue(fp, 1024, ':', [&](char const *key, char const *value) -> bool {
    if (strcmp(key, "PPid") == 0) {
      ppid = std::strtol(value, nullptr, 0);
      return false;
    }
    return true;
  });

  std::fclose(fp);
  return ppid;
}

bool ProcFS::GetProcessELFInfo(pid_t pid, ELFInfo &info) {
  //
  // On Linux, due to the binfmt_misc module, we need to
  // check that the target binary is really an ELF process
  // and it's for the target architecture.
  //
  union {
    Elf32_Ehdr e32;
    Elf64_Ehdr e64;
  } ehdr;

  int fd = ProcFS::OpenFd(pid, "exe");
  if (fd < 0)
    return false;

  //
  // Read the ELF header, that is enough.
  //
  ssize_t nread = ::read(fd, &ehdr, sizeof(ehdr));
  ::close(fd);

  //
  // Validate ELF header.
  //
  if (nread != sizeof(ehdr))
    return false;

  if (ehdr.e32.e_ident[EI_MAG0] != ELFMAG0 ||
      ehdr.e32.e_ident[EI_MAG1] != ELFMAG1 ||
      ehdr.e32.e_ident[EI_MAG2] != ELFMAG2 ||
      ehdr.e32.e_ident[EI_MAG3] != ELFMAG3)
    return false;

  info.machine = EM_NONE;
  info.endian = kEndianUnknown;
  info.is64Bit = false;

  if (ehdr.e32.e_ident[EI_CLASS] == ELFCLASS32) {
    info.machine = ehdr.e32.e_machine;
    info.endian =
        (ehdr.e32.e_ident[EI_DATA] == ELFDATA2LSB) ? kEndianLittle : kEndianBig;
    info.is64Bit = false;
    return true;
  } else if (ehdr.e32.e_ident[EI_CLASS] == ELFCLASS64) {
    info.machine = ehdr.e64.e_machine;
    info.endian =
        (ehdr.e64.e_ident[EI_DATA] == ELFDATA2LSB) ? kEndianLittle : kEndianBig;
    info.is64Bit = true;
    return true;
  }
  return false;
}

int32_t ProcFS::GetProcessELFMachineType(pid_t pid, bool *is64Bit) {
  ELFInfo info;
  if (!GetProcessELFInfo(pid, info))
    return -1;

  if (is64Bit != nullptr) {
    *is64Bit = info.is64Bit;
  }
  return info.machine;
}

//
// Returns the Mach-O kCPUTypexyz from the ELF machine
// type.
//
CPUType ProcFS::GetProcessCPUType(pid_t pid) {
  ELFInfo info;

  if (!GetProcessELFInfo(pid, info))
    return kCPUTypeAny;

  CPUType type;
  CPUSubType subtype;
  ELFSupport::MachineTypeToCPUType(info.machine, info.is64Bit, type, subtype);
  return type;
}

std::string ProcFS::GetProcessExecutablePath(pid_t pid) {
  char path[PATH_MAX + 1];
  char exe_path[PATH_MAX + 1];

  MakePath(path, PATH_MAX, pid, "exe");
  memset(exe_path, 0, sizeof(exe_path));
  if (readlink(path, exe_path, PATH_MAX) < 0)
    return std::string();

  return exe_path;
}

std::string ProcFS::GetProcessExecutableName(pid_t pid) {
  std::string name(GetProcessExecutablePath(pid));
  if (name.empty())
    return std::string();

  return basename(&name[0]);
}

bool ProcFS::GetProcessArguments(pid_t pid, StringCollection &args) {
  FILE *fp = ProcFS::OpenFILE(pid, "cmdline");
  if (fp == nullptr)
    return false;

  args.clear();

  std::string arg;
  for (;;) {
    char buf[1024], *end, *bp;
    size_t nread = fread(buf, 1, sizeof(buf), fp);
    if (nread == 0) {
      if (!arg.empty()) {
        args.push_back(arg);
      }
      break;
    }

    bp = buf;
    end = buf + nread;
    while (bp < end) {
      while (*bp != '\0') {
        arg += *bp++;
      }
      bp++;
      args.push_back(arg);
      arg.clear();
    }
  }

  std::fclose(fp);
  return true;
}

std::string ProcFS::GetProcessArgumentsAsString(pid_t pid, bool arg0) {
  StringCollection args;
  if (!GetProcessArguments(pid, args))
    return std::string();

  std::string result;
  bool is_arg0 = true;
  for (auto arg : args) {
    if (!arg0 && is_arg0) {
      is_arg0 = false;
      continue;
    }

    if (!result.empty()) {
      result += ' ';
    }

    bool quote = (arg.find(' ') != std::string::npos);
    if (quote) {
      result += '"';
    }

    // TODO more quoting
    result += arg;

    if (quote) {
      result += '"';
    }
  }

  return result;
}

std::string ProcFS::GetProcessName(pid_t pid) {
  return GetThreadName(pid, pid);
}

std::string ProcFS::GetThreadName(pid_t pid, pid_t tid) {
  std::string thread_name;
  FILE *fp;

  fp = ProcFS::OpenFILE(pid, tid, "status");
  if (fp == nullptr)
    return std::string();

  ParseKeyValue(fp, 1024, ':', [&](char const *key, char const *value) -> bool {
    if (strcmp(key, "Name") == 0) {
      thread_name = value;
      return false; // We're done, stop reading.
    }

    return true;
  });

  std::fclose(fp);
  return thread_name;
}

bool ProcFS::ReadProcessInfo(pid_t pid, ProcessInfo &info) {
  pid_t ppid;
  uid_t uid, euid;
  gid_t gid, egid;
  ELFInfo elf;
  std::string path;

  info.clear();

  if (!ReadProcessIds(pid, ppid, uid, euid, gid, egid) ||
      !GetProcessELFInfo(pid, elf) ||
      (path = GetProcessExecutablePath(pid)).empty())
    return false;

  info.pid = pid;
  info.parentPid = ppid;

  info.name.swap(path);

  info.realUid = uid;
  info.effectiveUid = euid;
  info.realGid = gid;
  info.effectiveGid = egid;

  ELFSupport::MachineTypeToCPUType(elf.machine, elf.is64Bit, info.cpuType,
                                   info.cpuSubType);

  info.nativeCPUType = elf.machine;
  info.nativeCPUSubType = kInvalidCPUType;

  info.endian = elf.endian;
  info.pointerSize = elf.is64Bit ? 8 : 4;

  info.osType = Platform::GetOSTypeName();
  info.osVendor = Platform::GetOSVendorName();

  return true;
}

bool ProcFS::EnumerateProcesses(bool allUsers, uid_t uid,
                                std::function<void(pid_t, uid_t)> const &cb) {
  DIR *dir = OpenDIR("");
  if (dir == nullptr)
    return false;

  while (struct dirent *dp = readdir(dir)) {
    pid_t pid = strtol(dp->d_name, nullptr, 0);
    if (pid == 0)
      continue;

    //
    // Get the uid.
    //
    struct stat stbuf;
    char path[PATH_MAX + 1];
    MakePath(path, PATH_MAX, pid, "");
    if (stat(path, &stbuf) < 0)
      continue;

    //
    // Compare if necessary.
    //
    if (!allUsers && stbuf.st_uid != uid)
      continue;

    //
    // We don't want kernel threads, so exclude them from the list,
    // we know they are kernel threads because "exe" points to nothing.
    //
    if (GetProcessExecutablePath(pid).empty())
      continue;

    cb(pid, stbuf.st_uid);
  }
  closedir(dir);

  return true;
}

bool ProcFS::EnumerateThreads(pid_t pid, std::function<void(pid_t)> const &cb) {
  DIR *dir = OpenDIR(pid, "task");
  if (dir == nullptr)
    return false;

  while (struct dirent *dp = readdir(dir)) {
    pid_t tid = strtol(dp->d_name, nullptr, 0);
    if (tid == 0)
      continue;

    cb(tid);
  }
  closedir(dir);

  return true;
}
} // namespace Linux
} // namespace Host
} // namespace ds2
