# ds2 [![Travis Build Status](https://travis-ci.org/facebook/ds2.svg?branch=master)](https://travis-ci.org/facebook/ds2/branches) [![CircleCI](https://circleci.com/gh/facebook/ds2.svg?style=shield)](https://circleci.com/gh/facebook/ds2) [![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/sdt15jwwbv2ocdlg/branch/master?svg=true)](https://ci.appveyor.com/project/a20012251/ds2/branch/master) [![Coverage Status](https://coveralls.io/repos/github/facebook/ds2/badge.svg?branch=master)](https://coveralls.io/github/facebook/ds2?branch=master)

ds2 is a debug server designed to be used with [LLDB](http://lldb.llvm.org/) to
perform remote debugging of Linux, Android, FreeBSD, Windows and Windows Phone
targets. Windows/Windows Phone support is still under active development.

## Running ds2

### Example

#### On the remote host

Launch ds2 with something like:

    $ ./ds2 gdbserver localhost:4242 /path/to/TestSimpleOutput

ds2 is now ready to accept connections on port 4242 from lldb.

#### On your local host

    $ lldb /path/to/TestSimpleOutput
    Current executable set to '/path/to/TestSimpleOutput' (x86_64).
    (lldb) gdb-remote localhost:4242
    Process 8336 stopped
    * thread #1: tid = 8336, 0x00007ffff7ddb2d0, name = 'TestSimpleOutput', stop reason = signal SIGTRAP
        frame #0: 0x00007ffff7ddb2d0
    -> 0x7ffff7ddb2d0:  movq   %rsp, %rdi
       0x7ffff7ddb2d3:  callq  0x7ffff7ddea70
       0x7ffff7ddb2d8:  movq   %rax, %r12
       0x7ffff7ddb2db:  movl   0x221b17(%rip), %eax
    (lldb) b main
    Breakpoint 1: where = TestSimpleOutput`main + 29 at TestSimpleOutput.cpp:6, address = 0x000000000040096d
    [... debug debug ...]
    (lldb) c
    Process 8336 resuming
    Process 8336 exited with status = 0 (0x00000000)
    (lldb)

### Command-Line Options

ds2 accepts the following options:

```
usage: ds2 [RUN_MODE] [OPTIONS] [[HOST]:PORT] [-- PROGRAM [ARGUMENTS...]]
  -a, --attach ARG           attach to the name or PID specified
  -f, --daemonize            detach and become a daemon
  -d, --debug                enable debug log output
  -F, --fd ARG               use a file descriptor to communicate
  -g, --gdb-compat           force ds2 to run in gdb compat mode
  -o, --log-file ARG         output log messages to the file specified
  -N, --named-pipe ARG       determine a port dynamically and write back to FIFO
  -n, --no-colors            disable colored output
  -D, --remote-debug         enable log for remote protocol packets
  -R, --reverse-connect      connect back to the debugger at [HOST]:PORT
  -e, --set-env ARG...       add an element to the environment before launch
  -S, --setsid               make ds2 run in its own session
  -E, --unset-env ARG...     remove an element from the environment before lauch
  -l, --listen ARG           specify the [host]:port to listen on
  [host]:port                the [host]:port to connect to
```

After building ds2 for your target, run it with the binary to debug, or attach
to an already running process. Then, start LLDB as usual and attach to the ds2
instance with the `gdb-remote` command.

The run mode and port number must be specified, where run mode is either
`g[dbserver]` or `p[latform]`. In most cases, the `g[dbserver]` option is desired.

## Building ds2

ds2 uses [CMake](http://www.cmake.org/) to generate its build system. A variety
of CMake toolchain files are provided to help with cross compilation for other
targets.

### Requirements

ds2 needs cmake, a C++11 compiler, flex and bison.

### Compiling on Linux, FreeBSD and macOS

After cloning the ds2 repository, run the following commands to build for the
current host:

```sh
cd ds2
mkdir build && cd build
cmake ..
make
```

### Compiling on Windows

ds2 builds on Windows using Visual Studio. You'll need:

* Windows builds of CMake for which you can grab binaries
  [here](https://cmake.org/download/);
* flex and bison binaries -- easiest way to get these is to install them with
  Cygwin and add that to your `PATH` (usually `C:\cygwin\bin`);
* an install of Visual Studio -- we use VS2015, but VS2013 should work too.

Then, as with linux, use CMake to generate the build system, then use msbuild
(or the script provided) to build the binary:

```sh
cd ds2
mkdir build && cd build
cmake ..
..\Support\Scripts\build-windows.bat
```

### Cross compiling for Windows Phone

ds2 can be built for Windows Phone, which will generate a dll that can later be
loaded in by the application we are debugging as a separate process. This needs
Visual Studio 2015.

To build for Windows Phone, use the `Toolchain-WinStore.cmake` toolchain file
as well as the "Visual Studio 14 2015 ARM" CMake generator.

```sh
cd ds2
mkdir build && cd build
cmake -G "Visual Studio 14 2015 ARM" -DCMAKE_TOOLCHAIN_FILE=../Support/CMake/Toolchain-WinStore.cmake" ..
..\Support\Scripts\build-windows.bat
```

### Cross compiling for Android

For Android native debugging, it is possible to build ds2 with the Android NDK.
A script is provided to download the Android NDK automatically for you.

`Support/Scripts/prepare-android-ndk.py` will download a working version
of the NDK, extract it, and install it to `/tmp/android-ndk`.

```sh
cd ds2
./Support/Scripts/prepare-android-ndk.py
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../Support/CMake/Toolchain-Android-ARM.cmake ..
make
```

Note that this will build ds2 targeting the highest level API level that the
NDK supports. If you want to target another api level, e.g. 21, add the flag
`-DCMAKE_SYSTEM_VERSION=21` to your cmake invocation.

#### Testing on Android device

If you would like to use ds2 to run tests in the LLDB test suite using an
Android device, you should use the script
`Support/Scripts/prepare-android-ndk.py` to get a checkout of the android NDK.
The LLDB test suite expects an NDK to exist on your host, and that script will
download and unpack it where the CMake Toolchain files expect it to be.

### Cross compiling for Linux-ARM

Cross-compiling for Linux-ARM is also possible. On Ubuntu 14.04, install an arm
toolchain (for instance `g++-4.8-arm-linux-gnueabihf`) and use the provided
toolchain file.

```sh
cd ds2
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../Support/CMake/Toolchain-Linux-ARM.cmake ..
make
```

This will generate a binary that you can copy to your device to start
debugging.

## Join the ds2 community

See the `CONTRIBUTING.md` file for how to help out.

## License

ds2 is licensed under the University of Illinois/NCSA Open Source License.

We also provide an additional patent grant which can be found in the `PATENTS`
file in the root directory of this source tree.

regsgen2, a tool used to generate register definitions is also licensed under
the University of Illinois/NCSA Open Source License and uses a json library
licensed under the Boost Software License. A complete copy of the latter can be
found in `Tools/libjson/LICENSE_1_0.txt`.
