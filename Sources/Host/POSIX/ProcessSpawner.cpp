//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#define __DS2_LOG_CLASS_NAME__ "ProcessSpawner"

#if defined(__linux__)
#include "DebugServer2/Host/Linux/ExtraSyscalls.h"
#endif
#include "DebugServer2/Host/ProcessSpawner.h"
#include "DebugServer2/Log.h"

#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>

using ds2::Host::ProcessSpawner;
using ds2::ErrorCode;

static bool open_terminal(int fds[2]) {
  char const *slave;

  fds[0] = ::posix_openpt(O_RDWR | O_NOCTTY);
  if (fds[0] == -1)
    goto error;

  if (::grantpt(fds[0]) == -1)
    goto error_fd0;

  if (::unlockpt(fds[0]) == -1)
    goto error_fd0;

  slave = ::ptsname(fds[0]);
  if (slave == nullptr)
    goto error_fd0;

  fds[1] = ::open(slave, O_RDWR);
  if (fds[1] == -1)
    goto error_fd0;

  return true;

error_fd0:
  ::close(fds[0]);
  fds[0] = -1;
error:
  return false;
}

static inline void close_terminal(int fds[2]) {
  ::close(fds[0]);
  ::close(fds[1]);
}

ProcessSpawner::ProcessSpawner() : _exitStatus(0), _signalCode(0), _pid(0) {}

bool ProcessSpawner::setExecutable(std::string const &path) {
  if (_pid != 0)
    return false;

  _executablePath = path;
  return true;
}

bool ProcessSpawner::setArguments(StringCollection const &args) {
  if (_pid != 0)
    return false;

  _arguments = args;
  return true;
}

bool ProcessSpawner::setWorkingDirectory(std::string const &path) {
  if (_pid != 0)
    return false;

  _workingDirectory = path;
  return true;
}

//
// Console redirection
//
bool ProcessSpawner::redirectInputToConsole() {
  if (_pid != 0)
    return false;

  _descriptors[0].mode = kRedirectConsole;
  _descriptors[0].delegate = nullptr;
  _descriptors[0].fd = -1;
  _descriptors[0].path.clear();
  return true;
}

bool ProcessSpawner::redirectOutputToConsole() {
  if (_pid != 0)
    return false;

  _descriptors[1].mode = kRedirectConsole;
  _descriptors[1].delegate = nullptr;
  _descriptors[1].fd = -1;
  _descriptors[1].path.clear();
  return true;
}

bool ProcessSpawner::redirectErrorToConsole() {
  if (_pid != 0)
    return false;

  _descriptors[2].mode = kRedirectConsole;
  _descriptors[2].delegate = nullptr;
  _descriptors[2].fd = -1;
  _descriptors[2].path.clear();
  return true;
}

//
// Null redirection
//
bool ProcessSpawner::redirectInputToNull() {
  if (_pid != 0)
    return false;

  _descriptors[0].mode = kRedirectNull;
  _descriptors[0].delegate = nullptr;
  _descriptors[0].fd = -1;
  _descriptors[0].path.clear();
  return true;
}

bool ProcessSpawner::redirectOutputToNull() {
  if (_pid != 0)
    return false;

  _descriptors[1].mode = kRedirectNull;
  _descriptors[1].delegate = nullptr;
  _descriptors[1].fd = -1;
  _descriptors[1].path.clear();
  return true;
}

bool ProcessSpawner::redirectErrorToNull() {
  if (_pid != 0)
    return false;

  _descriptors[2].mode = kRedirectNull;
  _descriptors[2].delegate = nullptr;
  _descriptors[2].fd = -1;
  _descriptors[2].path.clear();
  return true;
}

//
// File redirection
//
bool ProcessSpawner::redirectInputToFile(std::string const &path) {
  if (_pid != 0 || path.empty())
    return false;

  _descriptors[0].mode = kRedirectFile;
  _descriptors[0].delegate = nullptr;
  _descriptors[0].fd = -1;
  _descriptors[0].path = path;
  return true;
}

bool ProcessSpawner::redirectOutputToFile(std::string const &path) {
  if (_pid != 0 || path.empty())
    return false;

  _descriptors[1].mode = kRedirectFile;
  _descriptors[1].delegate = nullptr;
  _descriptors[1].fd = -1;
  _descriptors[1].path = path;
  return true;
}

bool ProcessSpawner::redirectErrorToFile(std::string const &path) {
  if (_pid != 0 || path.empty())
    return false;

  _descriptors[2].mode = kRedirectFile;
  _descriptors[2].delegate = nullptr;
  _descriptors[2].fd = -1;
  _descriptors[2].path = path;
  return true;
}

//
// Buffer redirection
//
bool ProcessSpawner::redirectOutputToBuffer() {
  if (_pid != 0)
    return false;

  _descriptors[1].mode = kRedirectBuffer;
  _descriptors[1].delegate = nullptr;
  _descriptors[1].fd = -1;
  _descriptors[1].path.clear();
  return true;
}

bool ProcessSpawner::redirectErrorToBuffer() {
  if (_pid != 0)
    return false;

  _descriptors[2].mode = kRedirectBuffer;
  _descriptors[2].delegate = nullptr;
  _descriptors[2].fd = -1;
  _descriptors[2].path.clear();
  return true;
}

//
// Delegate redirection
//
bool ProcessSpawner::redirectOutputToDelegate(RedirectDelegate delegate) {
  if (_pid != 0 || delegate == nullptr)
    return false;

  _descriptors[1].mode = kRedirectDelegate;
  _descriptors[1].delegate = delegate;
  _descriptors[1].fd = -1;
  _descriptors[1].path.clear();
  return true;
}

bool ProcessSpawner::redirectErrorToDelegate(RedirectDelegate delegate) {
  if (_pid != 0 || delegate == nullptr)
    return false;

  _descriptors[2].mode = kRedirectDelegate;
  _descriptors[2].delegate = delegate;
  _descriptors[2].fd = -1;
  _descriptors[2].path.clear();
  return true;
}

#define RD 0
#define WR 1

ErrorCode ProcessSpawner::run(std::function<bool()> preExecAction) {
  if (_pid != 0 || _executablePath.empty())
    return kErrorInvalidArgument;

  //
  // If we have redirection, prepare pipes and handles.
  //
  int fds[3][2] = {{-1, -1}, {-1, -1}, {-1, -1}};
  int term[2] = {-1, -1};
  bool startRedirectThread = false;

  for (size_t n = 0; n < 3; n++) {
    switch (_descriptors[n].mode) {
    case kRedirectConsole:
      // do nothing
      break;

    case kRedirectNull:
      if (n == 0) {
        fds[n][RD] = ::open("/dev/null", O_RDONLY);
      } else {
        fds[n][WR] = ::open("/dev/null", O_WRONLY);
      }
      break;

    case kRedirectFile:
      if (n == 0) {
        fds[n][RD] = ::open(_descriptors[n].path.c_str(), O_RDONLY);
      } else {
        fds[n][WR] = ::open(_descriptors[n].path.c_str(), O_RDWR);
        if (fds[n][WR] < 0) {
          fds[n][WR] =
              ::open(_descriptors[n].path.c_str(), O_CREAT | O_RDWR, 0600);
        }
      }
      break;

    case kRedirectBuffer:
      _outputBuffer.clear();
    // fall-through
    case kRedirectDelegate:
      startRedirectThread = true;
      if (term[0] == -1) {
        if (!open_terminal(term)) {
          return kErrorTooManyFiles;
        }
      }
      fds[n][RD] = term[RD];
      fds[n][WR] = term[WR];
      break;
    }
  }

  _pid = ::fork();
  if (_pid < 0) {
    close_terminal(term);
    return kErrorNoMemory;
  }

  if (_pid == 0) {
    if (::setgid(::getgid()) == 0) {
      ::setsid();

      for (size_t n = 0; n < 3; n++) {
        switch (_descriptors[n].mode) {
        case kRedirectConsole:
          // do nothing
          break;

        default:
          if (n == 0) {
            if (fds[n][WR] != -1) {
              ::close(fds[n][WR]);
            }
            ::dup2(fds[n][RD], 0);
            ::close(fds[n][RD]);
          } else {
            if (fds[n][RD] != -1) {
              ::close(fds[n][RD]);
            }
            ::dup2(fds[n][WR], n);
            ::close(fds[n][WR]);
          }
          break;
        }
      }

      if (!_workingDirectory.empty()) {
        ::chdir(_workingDirectory.c_str());
      }

      std::vector<char *> argv;
      argv.push_back(::basename(const_cast<char *>(&_executablePath[0])));
      for (auto const &arg : _arguments) {
        argv.push_back(const_cast<char *>(arg.c_str()));
      }
      argv.push_back(nullptr);
      if (!preExecAction())
        return kErrorUnknown;
      if (::execv(_executablePath.c_str(), &argv[0]) < 0) {
        DS2LOG(Main, Error, "cannot spawn executable %s, error=%s",
               _executablePath.c_str(), strerror(errno));
      }
    }
    ::_exit(127);
  }

  for (size_t n = 0; n < 3; n++) {
    switch (_descriptors[n].mode) {
    case kRedirectConsole:
      // do nothing
      break;

    default:
      if (n == 0) {
        if (fds[n][RD] != -1) {
          ::close(fds[n][RD]);
          _descriptors[0].fd = fds[n][WR];
        }
      } else {
        if (fds[n][WR] != -1) {
          ::close(fds[n][WR]);
          _descriptors[n].fd = fds[n][RD];
        }
      }
      break;
    }
  }

  if (startRedirectThread) {
    _delegateThread =
        std::move(std::thread(&ProcessSpawner::redirectionThread, this));
  }

  return kSuccess;
}

ErrorCode ProcessSpawner::wait() {
  if (_pid == 0)
    return kErrorProcessNotFound;

  int status = 0;
  pid_t pid = 0;
  for (;;) {
    pid = ::waitpid(_pid, &status, 0);
    if (pid != _pid) {
      switch (errno) {
      case EINTR:
        continue;
      case ESRCH:
        return kErrorProcessNotFound;
      default:
        return kErrorInvalidArgument;
      }
    }
    break;
  }

  //
  // Wait also the termination of the thread.
  //
  _delegateThread.join();

  _pid = 0;
  if (WIFEXITED(status)) {
    _exitStatus = WEXITSTATUS(status);
    _signalCode = 0;
  } else {
    _exitStatus = 0;
    _signalCode = WTERMSIG(status);
  }

  return kSuccess;
}

bool ProcessSpawner::isRunning() const {
  if (_pid == 0)
    return false;

  int status;
  return (::waitpid(_pid, &status, WNOHANG) == -1 && errno != ECHILD);
}

//
// Redirector Thread
//
void ProcessSpawner::redirectionThread() {
  for (;;) {
    struct pollfd pfds[3];
    struct pollfd *ppfds = pfds;
    int nfds = 0;

    std::memset(pfds, 0, sizeof(pfds));
    for (size_t n = 0; n < 3; n++) {
      switch (_descriptors[n].mode) {
      case kRedirectBuffer:
      case kRedirectDelegate:
        ppfds->fd = _descriptors[n].fd;
        ppfds->events = (n == 0) ? POLLOUT : POLLIN;
        ppfds++, nfds++;
        break;

      default:
        break;
      }
    }

    nfds = ::poll(pfds, nfds, 100);
    if (nfds < 0)
      break;

    bool done = false;
    bool hup = false;
    for (int n = 0; n < nfds; n++) {
      if (pfds[n].revents & POLLHUP) {
        hup = true;
      }

      size_t index;
      RedirectDescriptor *descriptor;
      for (index = 0; index < 3; index++) {
        if (_descriptors[index].fd == pfds[n].fd) {
          descriptor = _descriptors + index;
          break;
        }
      }

      if (pfds[n].events & POLLIN) {
        if (pfds[n].revents & POLLIN) {
          char buf[128];
          ssize_t nread = ::read(pfds[n].fd, buf, sizeof(buf));
          if (nread > 0) {
            if (descriptor->mode == kRedirectBuffer) {
              _outputBuffer.insert(_outputBuffer.end(), &buf[0], &buf[nread]);
            } else {
              descriptor->delegate(buf, nread);
            }
            done = true;
          }
        }
      }
    }

    if (!done && hup)
      break;
  }

  for (size_t n = 0; n < 3; n++) {
    if (_descriptors[n].fd != -1) {
      ::close(_descriptors[n].fd);
      _descriptors[n].fd = -1;
    }
  }
}
