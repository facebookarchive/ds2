//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/BreakpointManager.h"
#include "DebugServer2/GDBRemote/DebugSessionImpl.h"
#include "DebugServer2/GDBRemote/PlatformSessionImpl.h"
#include "DebugServer2/GDBRemote/ProtocolHelpers.h"
#include "DebugServer2/GDBRemote/SlaveSessionImpl.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Host/QueueChannel.h"
#include "DebugServer2/Host/Socket.h"
#include "DebugServer2/SessionThread.h"
#include "DebugServer2/Utils/Log.h"
#include "DebugServer2/Utils/OptParse.h"
#include "DebugServer2/Utils/String.h"

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <iomanip>
#include <memory>
#include <set>
#include <string>
#if !defined(OS_WIN32)
#include <sys/stat.h>
#include <unistd.h>
#endif

using ds2::Host::Socket;
using ds2::Host::QueueChannel;
using ds2::Host::Platform;
using ds2::BreakpointManager;
using ds2::GDBRemote::Session;
using ds2::GDBRemote::SessionDelegate;
using ds2::GDBRemote::PlatformSessionImpl;
using ds2::GDBRemote::DebugSessionImpl;
using ds2::GDBRemote::SlaveSessionImpl;

static std::string gDefaultPort = "12345";
static std::string gDefaultHost = "127.0.0.1";
static bool gKeepAlive = false;
static bool gGDBCompat = false;

// This function creates and initializes a socket than can be either a client
// (if reverse is true), or a server (if reverse is false) that will then need
// to be accept()'d on.
Socket *CreateSocket(std::string const &host, std::string const &port,
                     bool reverse) {
  auto socket = new Socket;

  if (reverse) {
    if (!socket->connect(host, port)) {
      DS2LOG(Fatal, "cannot connect to [%s:%s]: %s", host.c_str(), port.c_str(),
             socket->error().c_str());
    } else {
      DS2LOG(Info, "connected to [%s:%s]", socket->address().c_str(),
             socket->port().c_str());
    }
  } else {
    if (!socket->listen(host, port)) {
      DS2LOG(Fatal, "cannot listen on [%s:%s]: %s", host.c_str(), port.c_str(),
             socket->error().c_str());
    } else {
      DS2LOG(Info, "listening on [%s:%s]", socket->address().c_str(),
             socket->port().c_str());
    }
  }

  return socket;
}

#if !defined(OS_WIN32)
static int PlatformMain(std::string const &host, std::string const &port) {
  auto server = std::unique_ptr<Socket>(CreateSocket(host, port, false));

  PlatformSessionImpl impl;

  do {
    // Platform mode implies that we are talking to an LLDB remote.
    Session session(ds2::GDBRemote::kCompatibilityModeLLDB);
    session.setDelegate(&impl);
    auto client = std::unique_ptr<Socket>(server->accept());
    session.create(client.get());

    while (session.receive(/*cooked=*/false))
      continue;
  } while (gKeepAlive);

  return EXIT_SUCCESS;
}
#endif

static int RunDebugServer(Socket *socket, SessionDelegate *impl) {
  Session session(gGDBCompat ? ds2::GDBRemote::kCompatibilityModeGDB
                             : ds2::GDBRemote::kCompatibilityModeLLDB);
  auto qchannel = new QueueChannel(socket);
  SessionThread thread(qchannel, &session);

  session.setDelegate(impl);
  session.create(qchannel);

  DS2LOG(Debug, "DEBUG SERVER STARTED");
  thread.start();
  while (session.receive(/*cooked=*/true))
    continue;
  DS2LOG(Debug, "DEBUG SERVER KILLED");

  return EXIT_SUCCESS;
}

static int DebugMain(ds2::StringCollection const &args,
                     ds2::EnvironmentBlock const &env, int attachPid,
                     std::string const &host, std::string const &port,
                     bool reverse, std::string const &namedPipePath) {
  // Use a shared_ptr here so we can easily copy it before calling
  // RunDebugServer below, if this is a reverse connect case.
  auto socket = std::shared_ptr<Socket>(CreateSocket(host, port, reverse));

  if (!namedPipePath.empty()) {
    std::string realPort = socket->port();
    FILE *namedPipe = fopen(namedPipePath.c_str(), "a");
    if (namedPipe == nullptr) {
      DS2LOG(Error, "unable to open %s: %s", namedPipePath.c_str(),
             strerror(errno));
    } else {
      // Write the null terminator to the file. This follows the llgs behavior.
      fwrite(realPort.c_str(), 1, realPort.length() + 1, namedPipe);
      fclose(namedPipe);
    }
  }

  do {
    DebugSessionImpl *impl;

    if (attachPid > 0)
      impl = new DebugSessionImpl(attachPid);
    else if (args.size() > 0)
      impl = new DebugSessionImpl(args, env);
    else
      impl = new DebugSessionImpl();

    auto client = reverse ? socket : std::shared_ptr<Socket>(socket->accept());
    return RunDebugServer(client.get(), impl);

    delete impl;
  } while (gKeepAlive);

  return EXIT_SUCCESS;
}

#if !defined(OS_WIN32)
static int SlaveMain() {
  auto server = std::unique_ptr<Socket>(CreateSocket("localhost", "0", false));
  std::string port = server->port();

  pid_t pid = ::fork();
  if (pid < 0) {
    DS2LOG(Fatal, "cannot fork: %s", strerror(errno));
  }

  if (pid == 0) {
    //
    // Let the slave have its own session
    // TODO maybe should be a command line switch?
    //
    setsid();

    //
    // When in slave mode, output is suppressed but
    // for standard error.
    //
    close(0);
    close(1);

    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);

    auto client = std::unique_ptr<Socket>(server->accept());
    return RunDebugServer(client.get(), new SlaveSessionImpl);
  } else {
    //
    // Write to the standard output to let know our parent
    // where we're listening.
    //
    fprintf(stdout, "%s %d\n", port.c_str(), pid);
  }

  return EXIT_SUCCESS;
}
#endif

DS2_ATTRIBUTE_NORETURN static void ListProcesses() {
  printf("Processes running on %s:\n\n", Platform::GetHostName());
  printf("%s\n%s\n", "PID    USER       ARCH    NAME",
         "====== ========== ======= ============================");

  Platform::EnumerateProcesses(
      true, ds2::UserId(), [&](ds2::ProcessInfo const &info) {
        std::string pid = ds2::ToString(info.pid);

        std::string user;
        if (!Platform::GetUserName(info.realUid, user)) {
#if !defined(OS_WIN32)
          user = ds2::ToString(info.realUid);
#else
          user = "<NONE>";
#endif
        }

        std::string path = info.name;
        size_t lastsep;
#if defined(OS_WIN32)
        lastsep = path.rfind('\\');
#else
        lastsep = path.rfind('/');
#endif
        if (lastsep != std::string::npos) {
          path = path.substr(lastsep + 1);
        }

        printf("%-6s %-10.10s %-7.7s %s\n", pid.c_str(), user.c_str(),
               ds2::GetArchName(info.cpuType, info.cpuSubType), path.c_str());
      });

  exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  ds2::OptParse opts;
  int idx;

  ds2::Host::Platform::Initialize();
#if !defined(OS_WIN32)
  ds2::SetLogColorsEnabled(isatty(fileno(stderr)));
#endif
  ds2::SetLogLevel(ds2::kLogLevelInfo);

  enum RunMode {
    kRunModeNormal,
#if !defined(OS_WIN32)
    kRunModePlatform,
    kRunModeSlave,
#endif
  };

  int attachPid = -1;
  std::string port;
  std::string host;
  std::string namedPipePath;
  RunMode mode = kRunModeNormal;
  bool reverse = false;

  // Logging options.
  opts.addOption(ds2::OptParse::stringOption, "log-output", 'o',
                 "output log messages to the file specified");
  opts.addOption(ds2::OptParse::boolOption, "debug-remote", 'D',
                 "enable log for remote protocol packets");
  opts.addOption(ds2::OptParse::boolOption, "debug", 'd',
                 "enable debug log output");
  opts.addOption(ds2::OptParse::boolOption, "no-colors", 'n',
                 "disable colored output");

  // Non-debugserver options.
  opts.addOption(ds2::OptParse::boolOption, "list-processes", 'L',
                 "list processes debuggable by the current user");

  // Target debug options.
  opts.addOption(ds2::OptParse::vectorOption, "set-env", 'e',
                 "add an element to the environment before launch");
  opts.addOption(ds2::OptParse::vectorOption, "unset-env", 'E',
                 "remove an element from the environment before lauch");
  opts.addOption(ds2::OptParse::stringOption, "attach", 'a',
                 "attach to the name or PID specified");
  opts.addOption(ds2::OptParse::boolOption, "reverse-connect", 'R',
                 "connect back to the debugger at [HOST]:PORT");
  opts.addOption(ds2::OptParse::boolOption, "keep-alive", 'k',
                 "keep the server alive after the client disconnects");

  // lldb-server compatibility options.
  opts.addOption(ds2::OptParse::boolOption, "gdb-compat", 'g',
                 "force ds2 to run in gdb compat mode");
  opts.addOption(ds2::OptParse::stringOption, "named-pipe", 'N',
                 "determine a port dynamically and write back to FIFO");
  opts.addOption(ds2::OptParse::boolOption, "native-regs", 'r',
                 "use native registers (no-op)", true);
  opts.addOption(ds2::OptParse::boolOption, "setsid", 'S',
                 "make ds2 run in its own session (no-op)", true);

  if (argc < 2)
    opts.usageDie("first argument must be g[dbserver] or p[latform]");

  switch (argv[1][0]) {
  case 'g':
    mode = kRunModeNormal;
    break;
#if !defined(OS_WIN32)
  case 'p':
    mode = kRunModePlatform;
    break;
  case 's':
    mode = kRunModeSlave;
    break;
#endif
  default:
    opts.usageDie("first argument must be g[dbserver] or p[latform]");
  }

  idx = opts.parse(argc, argv, host, port);
  argc -= idx, argv += idx;

  // Logging options.
  if (!opts.getString("log-output").empty()) {
    FILE *stream = fopen(opts.getString("log-output").c_str(), "a");
    if (stream == nullptr) {
      DS2LOG(Error, "unable to open %s for writing: %s",
             opts.getString("log-output").c_str(), strerror(errno));
    } else {
#if !defined(OS_WIN32)
      // When ds2 is spawned by an app (e.g.: on Android), it will run with the
      // app's user/group ID, and will create its log file owned by the app. By
      // default, the permissions will be 0600 (rw-------) which makes us
      // unable to get the log files. chmod() them to be able to access them.
      fchmod(fileno(stream), 0644);
#endif
      ds2::SetLogColorsEnabled(false);
      ds2::SetLogOutputStream(stream);
      ds2::SetLogLevel(ds2::kLogLevelDebug);
    }
  }

  if (opts.getBool("debug-remote")) {
    ds2::SetLogLevel(ds2::kLogLevelPacket);
  } else if (opts.getBool("debug")) {
    ds2::SetLogLevel(ds2::kLogLevelDebug);
  }

  if (opts.getBool("no-colors")) {
    ds2::SetLogColorsEnabled(false);
  }

  // Non-debugserver options.
  if (opts.getBool("list-processes")) {
    ListProcesses();
  }

  // Target debug options and program arguments.
  ds2::StringCollection args(&argv[0], &argv[argc]);
  if (mode != kRunModeNormal && !args.empty()) {
    opts.usageDie("PROGRAM and ARGUMENTS only supported in gdbserver mode");
  }

  ds2::EnvironmentBlock env;
  Platform::GetCurrentEnvironment(env);

  for (auto const &e : opts.getVector("set-env")) {
    char const *arg = e.c_str();
    char const *equal = strchr(arg, '=');
    if (equal == nullptr || equal == arg) {
      DS2LOG(Error, "trying to add invalid environment value '%s', skipping",
             e.c_str());
      continue;
    }
    env[std::string(arg, equal)] = equal + 1;
  }

  for (auto const &e : opts.getVector("unset-env")) {
    if (env.find(e) == env.end()) {
      DS2LOG(Warning, "trying to remove '%s' not present in the environment",
             e.c_str());
      continue;
    }
    env.erase(e);
  }

  if (!opts.getString("attach").empty()) {
    attachPid = atoi(opts.getString("attach").c_str());
  }

  gKeepAlive = opts.getBool("keep-alive");
#if !defined(OS_WIN32)
  if (mode == kRunModePlatform) {
    // The platform spawner should stay alive by default.
    gKeepAlive = true;
  }
#endif

  reverse = opts.getBool("reverse-connect");
  if (mode != kRunModeNormal && reverse) {
    opts.usageDie("reverse-connect only supported in gdbserver mode");
  }

  // lldb-server compatibilty options.
  gGDBCompat = opts.getBool("gdb-compat");
  if (mode == kRunModeNormal && gGDBCompat && args.empty() && attachPid < 0) {
    // In GDB compatibility mode, we need a process to attach to or a command
    // line so we can launch it.
    // In LLDB mode, we can launch the debug server without any of those two
    // things, and wait for an "A" command that specifies the command line to
    // use to launch the inferior.
    opts.usageDie("either a program or target PID is required in gdb mode");
  }

  // This is used for llgs testing. We determine a port number dynamically and
  // write it back to the FIFO passed as argument for the test harness to use
  // it.
  namedPipePath = opts.getString("named-pipe");

  // Default host and port options.
  if (port.empty()) {
    // If we have a named pipe, set the port to 0 to indicate that we should
    // dynamically allocate it and write it back to the FIFO.
    port = namedPipePath.empty() ? gDefaultPort : "0";
  }
  if (host.empty()) {
    host = gDefaultHost;
  }

  switch (mode) {
  case kRunModeNormal:
    return DebugMain(args, env, attachPid, host, port, reverse, namedPipePath);
#if !defined(OS_WIN32)
  case kRunModePlatform:
    return PlatformMain(host, port);
  case kRunModeSlave:
    return SlaveMain();
#endif
  default:
    DS2BUG("invalid run mode: %d", mode);
  }
}
