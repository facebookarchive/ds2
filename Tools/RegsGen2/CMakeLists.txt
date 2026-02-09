## Copyright (c) Meta Platforms, Inc. and affiliates.
##
## This source code is licensed under the Apache License v2.0 with LLVM
## Exceptions found in the LICENSE file in the root directory of this
## source tree.

cmake_minimum_required(VERSION 3.1.0)

project(RegsGen2)

set(REGSGEN2_SOURCES
    FlagSet.cpp
    GDBDefinitions.cpp
    GDBVectorSet.cpp
    LLDBDefinitions.cpp
    ParseConstants.cpp
    RegisterSet.cpp
    RegisterTemplate.cpp
    main.cpp
    )

add_executable(regsgen2 ${REGSGEN2_SOURCES})
set_property(TARGET regsgen2 PROPERTY CXX_STANDARD 11)
target_compile_options(regsgen2 PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-unused-function)

add_subdirectory(../JSObjects "${CMAKE_CURRENT_BINARY_DIR}/JSObjects")
target_link_libraries(regsgen2 jsobjects)
