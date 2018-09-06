//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Base.h"
#if defined(OS_LINUX)
#include "DebugServer2/Host/Linux/ExtraWrappers.h"
#endif
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Host/ProcessSpawner.h"
#include "DebugServer2/Utils/Log.h"
#include "DebugServer2/Utils/Stringify.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <libgen.h>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>

using ds2::Utils::Stringify;

namespace ds2 {
namespace Host {

static bool open_terminal(int fds[2]) {
#if defined(OS_FREEBSD) || defined(OS_DARWIN)
  char *slave;
#else
  char slave[PATH_MAX];
#endif

  fds[0] = ::posix_openpt(O_RDWR | O_NOCTTY);
  if (fds[0] == -1)
    goto error;

  if (::grantpt(fds[0]) == -1)
    goto error_fd0;

  if (::unlockpt(fds[0]) == -1)
    goto error_fd0;

#if defined(OS_FREEBSD) || defined(OS_DARWIN)
  slave = ptsname(fds[0]);
#else
  if (::ptsname_r(fds[0], slave, sizeof(slave)) != 0)
    goto error_fd0;
#endif

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

ProcessSpawner::ProcessSpawner()
    : _exitStatus(0), _signalCode(0), _pid(0), _shell(false) {}

ProcessSpawner::~ProcessSpawner() { flushAndExit(); }

void ProcessSpawner::flushAndExit() {
  if (_delegateThread.joinable())
    _delegateThread.join();
}

bool ProcessSpawner::setExecutable(std::string const &path) {
  if (_pid != 0)
    return false;

  _executablePath = path;
  _shell = false;
  return true;
}

bool ProcessSpawner::setShellCommand(std::string const &command) {
  if (_pid != 0) {
    return false;
  }

  setExecutable("sh");

  StringCollection args;
  args.push_back("-c");
  args.push_back(command);

  setArguments(args);

  _shell = true;
  return true;
}

bool ProcessSpawner::setArguments(StringCollection const &args) {
  if (_pid != 0)
    return false;

  _arguments = args;
  return true;
}

bool ProcessSpawner::setEnvironment(EnvironmentBlock const &env) {
  if (_pid != 0)
    return false;

  _environment = env;
  return true;
}

bool ProcessSpawner::addEnvironment(std::string const &key,
                                    std::string const &val) {
  if (_pid != 0)
    return false;

  _environment[key] = val;
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
  if (_pid != 0 || _descriptors[0].mode != kRedirectUnset)
    return false;

  _descriptors[0].mode = kRedirectConsole;
  _descriptors[0].delegate = nullptr;
  _descriptors[0].fd = -1;
  _descriptors[0].path.clear();
  return true;
}

bool ProcessSpawner::redirectOutputToConsole() {
  if (_pid != 0 || _descriptors[1].mode != kRedirectUnset)
    return false;

  _descriptors[1].mode = kRedirectConsole;
  _descriptors[1].delegate = nullptr;
  _descriptors[1].fd = -1;
  _descriptors[1].path.clear();
  return true;
}

bool ProcessSpawner::redirectErrorToConsole() {
  if (_pid != 0 || _descriptors[2].mode != kRedirectUnset)
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
  if (_pid != 0 || _descriptors[0].mode != kRedirectUnset)
    return false;

  _descriptors[0].mode = kRedirectNull;
  _descriptors[0].delegate = nullptr;
  _descriptors[0].fd = -1;
  _descriptors[0].path.clear();
  return true;
}

bool ProcessSpawner::redirectOutputToNull() {
  if (_pid != 0 || _descriptors[1].mode != kRedirectUnset)
    return false;

  _descriptors[1].mode = kRedirectNull;
  _descriptors[1].delegate = nullptr;
  _descriptors[1].fd = -1;
  _descriptors[1].path.clear();
  return true;
}

bool ProcessSpawner::redirectErrorToNull() {
  if (_pid != 0 || _descriptors[2].mode != kRedirectUnset)
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
  if (_pid != 0 || path.empty() || _descriptors[0].mode != kRedirectUnset)
    return false;

  _descriptors[0].mode = kRedirectFile;
  _descriptors[0].delegate = nullptr;
  _descriptors[0].fd = -1;
  _descriptors[0].path = path;
  return true;
}

bool ProcessSpawner::redirectOutputToFile(std::string const &path) {
  if (_pid != 0 || path.empty() || _descriptors[1].mode != kRedirectUnset)
    return false;

  _descriptors[1].mode = kRedirectFile;
  _descriptors[1].delegate = nullptr;
  _descriptors[1].fd = -1;
  _descriptors[1].path = path;
  return true;
}

bool ProcessSpawner::redirectErrorToFile(std::string const &path) {
  if (_pid != 0 || path.empty() || _descriptors[2].mode != kRedirectUnset)
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
  if (_pid != 0 || _descriptors[1].mode != kRedirectUnset)
    return false;

  _descriptors[1].mode = kRedirectBuffer;
  _descriptors[1].delegate = nullptr;
  _descriptors[1].fd = -1;
  _descriptors[1].path.clear();
  return true;
}

bool ProcessSpawner::redirectErrorToBuffer() {
  if (_pid != 0 || _descriptors[2].mode != kRedirectUnset)
    return false;

  _descriptors[2].mode = kRedirectBuffer;
  _descriptors[2].delegate = nullptr;
  _descriptors[2].fd = -1;
  _descriptors[2].path.clear();
  return true;
}

//
// Terminal input redirection
//
ErrorCode ProcessSpawner::input(ByteVector const &buf) {
  if (_pid == 0) {
    return kErrorProcessNotFound;
  }

  if (_descriptors[0].mode != kRedirectTerminal) {
    return kErrorInvalidArgument;
  }

  size_t size = buf.size();
  const unsigned char *data = buf.data();
  while (size > 0) {
    ssize_t nwritten = ::write(_descriptors[0].fd, data, size);
    if (nwritten < 0) {
      return Platform::TranslateError();
    }

    size -= nwritten;
    data += nwritten;
  }

  return kSuccess;
}

bool ProcessSpawner::redirectInputToTerminal() {
  if (_pid != 0 || _descriptors[0].mode != kRedirectUnset)
    return false;

  _descriptors[0].mode = kRedirectTerminal;
  _descriptors[0].delegate = nullptr;
  _descriptors[0].fd = -1;
  _descriptors[0].path.clear();
  return true;
}

//
// Delegate redirection
//
bool ProcessSpawner::redirectOutputToDelegate(RedirectDelegate delegate) {
  if (_pid != 0 || _descriptors[1].mode != kRedirectUnset)
    return false;

  _descriptors[1].mode = kRedirectDelegate;
  _descriptors[1].delegate = delegate;
  _descriptors[1].fd = -1;
  _descriptors[1].path.clear();
  return true;
}

bool ProcessSpawner::redirectErrorToDelegate(RedirectDelegate delegate) {
  if (_pid != 0 || _descriptors[2].mode != kRedirectUnset)
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
    // intentional fall-through to kRedirectConsole
    case kRedirectUnset:
      _descriptors[n].mode = kRedirectConsole;
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
      // Don't forget to set O_NOCTTY on these. If we run without a controlling
      // terminal (i.e.: we called `setsid()` during initialization), and if
      // the remote sends us the path to a virtual terminal (e.g.:
      // `/dev/pts/2`) to use as stdin/stdout/stderr, opening that terminal
      // makes it become our controlling terminal. We then die when closing it
      // later on.
      if (n == 0) {
        fds[n][RD] = ::open(_descriptors[n].path.c_str(), O_RDONLY | O_NOCTTY);
      } else {
        fds[n][WR] = ::open(_descriptors[n].path.c_str(), O_RDWR | O_NOCTTY);
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
    // fall-through
    case kRedirectTerminal:
      if (term[RD] == -1) {
        if (!open_terminal(term)) {
          DS2LOG(Error, "failed to open terminal: %s", Stringify::Errno(errno));
          return Platform::TranslateError();
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
    DS2LOG(Error, "failed to fork: %s", Stringify::Errno(errno));
    return Platform::TranslateError();
  }

  if (_pid == 0) {
    if (::setgid(::getgid()) == 0) {
      ::setsid();

      for (size_t n = 0; n < 3; n++) {
        switch (_descriptors[n].mode) {
        case kRedirectConsole:
          // do nothing
          break;

        case kRedirectDelegate:
        case kRedirectTerminal:
          //
          // We are using the same virtual terminal for all delegate
          // redirections, so dup2() only, do not close. We will close when all
          // FDs have been dup2()'d.
          //
          ::dup2(fds[n][WR], n);
          break;

        default:
          if (n == 0) {
            if (fds[n][WR] != -1) {
              ::close(fds[n][WR]);
            }
            ::dup2(fds[n][RD], n);
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

      close_terminal(term);

      if (!_workingDirectory.empty()) {
        int res = ::chdir(_workingDirectory.c_str());
        if (res != 0) {
          return Platform::TranslateError();
        }
      }

      std::vector<char *> args;
      args.push_back(const_cast<char *>(_executablePath.c_str()));
      for (auto const &e : _arguments)
        args.push_back(const_cast<char *>(e.c_str()));
      args.push_back(nullptr);

      std::vector<char *> environment;
      for (auto const &env : _environment)
        environment.push_back(const_cast<char *>(
            (new std::string(env.first + '=' + env.second))->c_str()));
      environment.push_back(nullptr);

      if (!preExecAction()) {
        DS2LOG(Error, "pre exec action failed");
        return kErrorUnknown;
      }

      if (_shell) {
        if (::execvp(_executablePath.c_str(), &args[0]) < 0) {
          DS2LOG(Error, "cannot run shell command %s, error=%s",
                 _executablePath.c_str(), strerror(errno));
        }
      } else {
        if (::execve(_executablePath.c_str(), &args[0], &environment[0]) < 0) {
          DS2LOG(Error, "cannot spawn executable %s, error=%s",
                 _executablePath.c_str(), strerror(errno));
        }
      }
    }
    ::_exit(127);
  }

  ::close(term[WR]);

  for (size_t n = 0; n < 3; n++) {
    switch (_descriptors[n].mode) {
    case kRedirectConsole:
      // do nothing
      break;

    case kRedirectTerminal:
    case kRedirectDelegate:
      _descriptors[n].fd = term[RD];
      break;

    default:
      if (n == 0) {
        if (fds[n][RD] != -1) {
          ::close(fds[n][RD]);
          _descriptors[n].fd = fds[n][WR];
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
    _delegateThread = std::thread(&ProcessSpawner::redirectionThread, this);
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
        ppfds++;
        nfds++;
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

      RedirectDescriptor *descriptor = nullptr;
      for (int index = 0; index < 3; index++) {
        if (_descriptors[index].fd == pfds[n].fd &&
            (_descriptors[index].mode == kRedirectBuffer ||
             _descriptors[index].mode == kRedirectDelegate)) {
          descriptor = &_descriptors[index];
          break;
        }
      }
      DS2ASSERT(descriptor != nullptr);

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

  for (auto &_descriptor : _descriptors) {
    if (_descriptor.fd != -1) {
      ::close(_descriptor.fd);
      _descriptor.fd = -1;
    }
  }
}
} // namespace Host
} // namespace ds2
