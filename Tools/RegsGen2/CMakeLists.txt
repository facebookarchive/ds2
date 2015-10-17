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

project(RegsGen2)

find_package(BISON)
find_package(FLEX)

FLEX_TARGET(JSON_SCANNER
            ${RegsGen2_SOURCE_DIR}/libjson/tokenizer.l
            ${RegsGen2_BINARY_DIR}/tokenizer.c)
BISON_TARGET(JSON_PARSER
             ${RegsGen2_SOURCE_DIR}/libjson/parser.y
             ${RegsGen2_BINARY_DIR}/parser.c
             COMPILE_FLAGS --no-lines)
ADD_FLEX_BISON_DEPENDENCY(JSON_SCANNER JSON_PARSER)

add_executable(regsgen2
               ${BISON_JSON_PARSER_OUTPUTS}
               ${FLEX_JSON_SCANNER_OUTPUTS}
               FlagSet.cpp
               GDBDefinitions.cpp
               GDBVectorSet.cpp
               JSObjects.cpp
               LLDBDefinitions.cpp
               ParseConstants.cpp
               RegisterSet.cpp
               RegisterTemplate.cpp
               libjson/json.c
               libjson/qstring.c
               main.cpp
               )

include_directories(${RegsGen2_SOURCE_DIR}/libjson)
include_directories(${RegsGen2_BINARY_DIR})

set(COMMON_FLAGS "-D_XOPEN_SOURCE=500 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 ${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ${COMMON_FLAGS}")
