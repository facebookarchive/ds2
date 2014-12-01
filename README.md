# ds2 [![Build Status](https://travis-ci.org/facebook/ds2.png?branch=master)](https://travis-ci.org/facebook/ds2)

ds2 is a debug server designed to be used with [LLDB](http://lldb.llvm.org/) to
perform remote debugging of Linux and android targets. It is still under active
development, and one medium-term goal is to support Windows debugging.

## Requirements

ds2 needs cmake and a C++11 compiler.

## Building ds2

ds2 uses [CMake](http://www.cmake.org/) to generate its build system. After
cloning the ds2 repository, run the following commands to build for the current
host:

```sh
cd ds2
mkdir build
cd build
cmake ..
make
```

### Cross compiling for Linux-ARM

Compiling for Linux-ARM (which can be then used on android devices) is also
possible. On Ubuntu 14.04, install `gcc-4.7-arm-linux-gnueabi` and
`g++-4.7-arm-linux-gnueabi` and use the provided toolchain file.

```sh
cd ds2
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../Support/CMake/Toolchain-Linux-ARM.cmake ..
make
```

This will generate a binary that you can drop on your device to start
debugging. Adding `-DSTATIC=1` to the CMake line will generate a static binary
instead, which might be useful if some of the libraries ds2 depends on are not
present on the remote device.

## Running ds2

ds2 accepts the following options:

    usage: ds2 [OPTIONS] [PROGRAM [ARGUMENTS...]]
      -a, --attach ARG          attach to the name or PID specified
      -R, --debug-remote        enable debugging of remote protocol
      -k, --keep-alive          keep the server alive after the client disconnects
      -L, --list-processes      list processes debuggable by the current user
      -o, --log-output ARG      output log message to the file specified
      -n, --no-colors           disable colored output
      -P, --platform            execute in platform mode
      -p, --port ARG            listen on the port specified
      -S, --slave               run in slave mode (used from platform spawner)

After building ds2 for your target, run it with the binary to debug, or attach
to an already running process. Then, start LLDB as usual and attach to the ds2
instance with the `gdb-remote` command.

ds2 listens on port 12345 by default; `--port` can be used to specify the port
number to use.

### Example

#### On the remote host

Launch ds2 with something like:

    $ ./ds2 --port 4242 ./TestSimpleOutput

ds2 is now ready to accept connections on port 4242 from lldb.

#### On your local host

    $ lldb /path/to/TestSimpleOutput
    Current executable set to '/path/to/TestSimpleOutput' (x86_64).
    (lldb) gdb-remote sas-ubuntu-vm.local:4242
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

## Join the ds2 community

See the CONTRIBUTING.md file for how to help out.

## License

ds2 is licensed under the University of Illinois/NCSA Open Source License.

We also provide an additional patent grant which can be found in the `PATENTS`
file in the root directory of this source tree.

regsgen2, a tool used to generate register definitions is also licensed under
the University of Illinois/NCSA Open Source License and uses a json library
licensed under the Boost Software License. A complete copy of the latter can be
found in `Tools/libjson/LICENSE_1_0.txt`.
