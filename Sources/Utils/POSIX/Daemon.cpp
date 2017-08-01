//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Utils/Daemon.h"
#include "DebugServer2/Utils/Log.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

namespace ds2 {
namespace Utils {

void Daemonize() {
  pid_t pid;

  pid = ::fork();
  if (pid < 0) {
    DS2LOG(Fatal, "cannot fork: %s", ::strerror(errno));
  } else if (pid > 0) {
    ::exit(EXIT_SUCCESS);
  }

  pid_t sid = ::setsid();
  if (sid < 0) {
    DS2LOG(Fatal, "cannot setsid: %s", ::strerror(errno));
  }

  pid = ::fork();
  if (pid < 0) {
    DS2LOG(Fatal, "cannot fork: %s", ::strerror(errno));
  } else if (pid > 0) {
    ::exit(EXIT_SUCCESS);
  }

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  open("/dev/null", O_RDONLY);
  open("/dev/null", O_WRONLY);
  open("/dev/null", O_WRONLY);
}
} // namespace Utils
} // namespace ds2
