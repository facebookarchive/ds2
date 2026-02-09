## Copyright (c) Meta Platforms, Inc. and affiliates.
##
## This source code is licensed under the Apache License v2.0 with LLVM
## Exceptions found in the LICENSE file in the root directory of this
## source tree.

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR ARM)

set(CMAKE_C_COMPILER arm-linux-gnueabi-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabi-g++)

set(TIZEN 1)
set(STATIC 1)
