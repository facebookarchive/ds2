## Copyright (c) Meta Platforms, Inc. and affiliates.
##
## This source code is licensed under the Apache License v2.0 with LLVM
## Exceptions found in the LICENSE file in the root directory of this
## source tree.

set(CMAKE_SYSTEM_NAME WindowsStore)
set(CMAKE_SYSTEM_VERSION 10.0)
set(CMAKE_GENERATOR_PLATFORM x64)

# For WinStore we build as a library because creating a process seems
# restricted, so we just have a separate process open `ds2.dll` and run `main`
# from it.
set(LIBRARY 1)
