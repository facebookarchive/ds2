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

#include "DebugServer2/Types.h"

#include <cstdio>
#include <dirent.h>
#include <fcntl.h>
#include <functional>
#include <sys/stat.h>
#include <unistd.h>

namespace ds2 {
namespace Host {
namespace Linux {

// On Linux, COMM field is 16 chars long
static size_t const kCOMMLengthMax = 16;

enum ProcState {
  kProcStateUninterruptible = 'D',
  kProcStateRunning = 'R',
  kProcStateSleeping = 'S',
  kProcStateStopped = 'T',
  kProcStateTraced = 't',
  kProcStatePaging = 'W',
  kProcStateDead = 'X',
  kProcStateZombie = 'Z',
};

class ProcFS {
public:
  struct Uptime {
    struct timespec run_time;
    struct timespec idle_time;
  };

  struct Stat {
    pid_t pid;
    char tcomm[kCOMMLengthMax + 1];
    char state;
    pid_t ppid;
    pid_t pgrp;
    pid_t sid;
    uint32_t tty_nr;
    pid_t tty_pgrp;
    uint64_t flags;
    uint64_t min_flt;
    uint64_t cmin_flt;
    uint64_t maj_flt;
    uint64_t cmaj_flt;
    uint64_t utime;
    uint64_t stime;
    uint64_t cutime;
    uint64_t cstime;
    int32_t priority;
    int32_t nice;
    uint32_t num_threads;
    uint64_t it_real_value;
    uint64_t start_time;
    uint64_t vsize;
    uint64_t rss;
    uint64_t rsslim;
    uint64_t start_code;
    uint64_t end_code;
    uint64_t start_stack;
    uint64_t esp;
    uint64_t eip;
    uint64_t pending;
    uint64_t blocked;
    uint64_t sigign;
    uint64_t sigcatch;
    uint64_t wchan;
    uint32_t exit_signal;
    uint32_t task_cpu;
    int32_t rt_priority;
    uint32_t policy;
    uint64_t blkio_ticks;
    uint64_t gtime;
    uint64_t cgtime;
    uint64_t start_data;
    uint64_t end_data;
    uint64_t start_brk;
  };

public:
  static int OpenFd(const char *what, int mode = O_RDONLY);
  static int OpenFd(pid_t pid, const char *what, int mode = O_RDONLY);
  static int OpenFd(pid_t pid, pid_t tid, const char *what,
                    int mode = O_RDONLY);

  static FILE *OpenFILE(char const *what, char const *mode = "r");
  static FILE *OpenFILE(pid_t pid, char const *what, char const *mode = "r");
  static FILE *OpenFILE(pid_t pid, pid_t tid, const char *what,
                        char const *mode = "r");

  static DIR *OpenDIR(const char *what);
  static DIR *OpenDIR(pid_t pid, const char *what);
  static DIR *OpenDIR(pid_t pid, pid_t tid, const char *what);

  static bool ReadLink(pid_t pid, char const *what, char *buf, size_t bufsiz);
  static bool ReadLink(pid_t pid, pid_t tid, char const *what, char *buf,
                       size_t bufsiz);

public:
  static void
  ParseKeyValue(FILE *fp, size_t maxsize, char sep,
                std::function<bool(char const *, char const *)> const &cb);
  static void ParseValues(FILE *fp, size_t maxsize, char sep, bool includeSep,
                          std::function<bool(size_t, char const *)> const &cb);

public:
  static bool ReadUptime(Uptime &uptime);
  static bool ReadStat(pid_t pid, Stat &stat);
  static bool ReadStat(pid_t pid, pid_t tid, Stat &stat);
  static bool ReadProcessIds(pid_t pid, pid_t &ppid, uid_t &uid, uid_t &euid,
                             gid_t &gid, gid_t &egid);

public:
  struct ELFInfo {
    uint32_t machine;
    Endian endian;
    bool is64Bit;
  };

  static bool GetProcessELFInfo(pid_t pid, ELFInfo &info);
  static int32_t GetProcessELFMachineType(pid_t pid, bool *is64Bit = nullptr);
  static CPUType GetProcessCPUType(pid_t pid);

public:
  static bool ReadProcessInfo(pid_t pid, ProcessInfo &info);

public:
  static std::string GetProcessName(pid_t pid);
  static pid_t GetProcessParentPid(pid_t pid);
  static std::string GetThreadName(pid_t pid, pid_t tid);
  static std::string GetProcessExecutableName(pid_t pid);
  static std::string GetProcessExecutablePath(pid_t pid);
  static bool GetProcessArguments(pid_t pid, ds2::StringCollection &args);
  static std::string GetProcessArgumentsAsString(pid_t pid, bool arg0 = false);

public:
  static bool EnumerateProcesses(bool allUsers, uid_t uid,
                                 std::function<void(pid_t, uid_t)> const &cb);
  static bool EnumerateThreads(pid_t pid, std::function<void(pid_t)> const &cb);
};
} // namespace Linux
} // namespace Host
} // namespace ds2
