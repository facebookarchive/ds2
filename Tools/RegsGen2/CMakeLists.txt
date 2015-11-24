##
## Copyright (c) 2014, Facebook, Inc.
## All rights reserved.
##
## This source code is licensed under the University of Illinois/NCSA Open
## Source License found in the LICENSE file in the root directory of this
## source tree. An additional grant of patent rights can be found in the
## PATENTS file in the same directory.
##

cmake_minimum_required(VERSION 2.8)

add_subdirectory(../JSObjects "${CMAKE_CURRENT_BINARY_DIR}/JSObjects")

project(RegsGen2)

add_executable(regsgen2
               FlagSet.cpp
               GDBDefinitions.cpp
               GDBVectorSet.cpp
               LLDBDefinitions.cpp
               ParseConstants.cpp
               RegisterSet.cpp
               RegisterTemplate.cpp
               main.cpp
               )

target_link_libraries(regsgen2 jsobjects)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function")
