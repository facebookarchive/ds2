##
## Copyright (c) 2014-present, Facebook, Inc.
## All rights reserved.
##
## This source code is licensed under the University of Illinois/NCSA Open
## Source License found in the LICENSE file in the root directory of this
## source tree. An additional grant of patent rights can be found in the
## PATENTS file in the same directory.
##

# To build a docker image using this file, run the following command:
#     docker build - < Support/Testing/CircleCI/Dockerfile
#
# After building the image, you will see a line like this at the end of the
# build process:
#     Successfully built 360abf8e6246
#
# This hash identifies the image you just built. You can then run it locally
# with the following command:
#     docker run --rm -it --name ds2-testing 360abf8e6246
#
# Publishing the image for use by CircleCI is done with:
#     docker tag 360abf8e6246 sas42/ds2-build-env
#     docker push sas42/ds2-build-env

FROM ubuntu:16.04
MAINTAINER Stephane Sezer <sas@fb.com>

# Install apt tools
RUN apt-get update
RUN apt-get install -y software-properties-common python-software-properties wget

# Add LLVM apt repos
RUN wget -O - "http://llvm.org/apt/llvm-snapshot.gpg.key" | apt-key add -
RUN add-apt-repository -y "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-5.0 main"
RUN apt-get update

# Install build dependencies
RUN apt-get install -y cmake ninja-build
RUN apt-get install -y flex bison

# Install x86 compilers
RUN apt-get install -y g++-4.9-multilib
RUN apt-get install -y clang-5.0

# Install arm compilers
RUN apt-get install -y g++-4.9-multilib-arm-linux-gnueabi

# Install mingw compilers
RUN apt-get install -y g++-mingw-w64-x86-64
RUN apt-get install -y g++-mingw-w64-i686
RUN update-alternatives --set i686-w64-mingw32-gcc /usr/bin/i686-w64-mingw32-gcc-posix
RUN update-alternatives --set i686-w64-mingw32-g++ /usr/bin/i686-w64-mingw32-g++-posix
RUN update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix
RUN update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix

# Install test dependencies
RUN apt-get install -y git lldb-5.0 gdb clang-format-5.0

# Install documentation dependencies
RUN apt-get install -y doxygen graphviz

# Create multilib symlink for gcc 4.9
RUN ln -s /usr/include/x86_64-linux-gnu/asm /usr/include/asm
