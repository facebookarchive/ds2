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

project(regsgen C CXX)

find_package(BISON)
find_package(FLEX)

include_directories(${regsgen_SOURCE_DIR}/libjson)
include_directories(${regsgen_BINARY_DIR})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_EXE_FLAGS "${CMAKE_EXE_FLAGS} -std=c++11")

FLEX_TARGET(JSON_SCANNER
            ${regsgen_SOURCE_DIR}/libjson/tokenizer.l
            ${regsgen_BINARY_DIR}/tokenizer.c)
BISON_TARGET(JSON_PARSER
             ${regsgen_SOURCE_DIR}/libjson/parser.y
             ${regsgen_BINARY_DIR}/parser.c)
ADD_FLEX_BISON_DEPENDENCY(JSON_SCANNER JSON_PARSER)

add_library(json_gna STATIC
            libjson/json.c
            libjson/qstring.c
            JSObjects.cpp
            ${BISON_JSON_PARSER_OUTPUTS}
            ${FLEX_JSON_SCANNER_OUTPUTS})

file(MAKE_DIRECTORY ${regsgen_BINARY_DIR}/test)
add_executable(test/jsontest libjson/jsontest.c)
target_link_libraries(test/jsontest json_gna)

add_executable(regsgen2
               main.cpp
               ParseConstants.cpp
               FlagSet.cpp
               GDBVectorSet.cpp
               RegisterTemplate.cpp
               RegisterSet.cpp
               LLDBDefinitions.cpp
               GDBDefinitions.cpp
               )
target_link_libraries(regsgen2 json_gna)
