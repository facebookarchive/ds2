# ds2 [![Travis Build Status](https://travis-ci.org/facebook/ds2.svg?branch=master)](https://travis-ci.org/facebook/ds2) [![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/sdt15jwwbv2ocdlg/branch/master?svg=true)](https://ci.appveyor.com/project/a20012251/ds2/branch/master)

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
  -d, --debug                enable debug log output
  -D, --debug-remote         enable log for remote protocol packets
  -g, --gdb-compat           force ds2 to run in gdb compat mode
  -k, --keep-alive           keep the server alive after the client disconnects
  -L, --list-processes       list processes debuggable by the current user
  -o, --log-file ARG         output log messages to the file specified
  -N, --named-pipe ARG       determine a port dynamically and write back to FIFO
  -n, --no-colors            disable colored output
  -R, --reverse-connect      connect back to the debugger at [HOST]:PORT
  -e, --set-env              add an element to the environment before launch
  -E, --unset-env            remove an element from the environment before lauch
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

### Compiling on Linux and FreeBSD

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

### Cross compiling for Android

For Android native debugging, it is possible to build ds2 with a toolchain from
the Android NDK. A script is provided to generate the toolchain, and a CMake
toolchain file can then be used to generate the build system.

`Support/Scripts/prepare-android-toolchain.sh` will download a working version
of the NDK, extract it, and install the toolchain to
`/tmp/android-ndk-arm-toolchain` which can then be used for the build.

```sh
cd ds2
./Support/Scripts/prepare-android-toolchain.sh arm
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../Support/CMake/Toolchain-Android-ARM.cmake ..
make
```

Builds of ds2 targetting Android generate a static binary by default. The final
build product can be copied over to the remote device and used with minimal
dependencies.

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
