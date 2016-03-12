##
## Copyright (c) 2014-present, Facebook, Inc.
## All rights reserved.
##
## This source code is licensed under the University of Illinois/NCSA Open
## Source License found in the LICENSE file in the root directory of this
## source tree. An additional grant of patent rights can be found in the
## PATENTS file in the same directory.
##

# This is meant to be used on Windows, to generate Visual Studio project files.
# We cannot specify the architure we are targetting (x86, arm, etc) here
# because that needs to be specified in the generator instead.
# For instance,
#     cmake -DCMAKE_TOOLCHAIN_FILE=../Support/CMake/Toolchain-WinStore.cmake -G"Visual Studio 14 2015" ..
# will build for WinStore x86, whereas
#     cmake -DCMAKE_TOOLCHAIN_FILE=../Support/CMake/Toolchain-WinStore.cmake -G"Visual Studio 14 2015 ARM" ..
# will build for WinStore ARM.
set(CMAKE_SYSTEM_NAME WindowsStore)
set(CMAKE_SYSTEM_VERSION 10.0)
