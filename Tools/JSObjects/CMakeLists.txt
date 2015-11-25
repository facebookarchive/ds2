##
## Copyright (c) 2014, Facebook, Inc.
## All rights reserved.
##
## This source code is licensed under the University of Illinois/NCSA Open
## Source License found in the LICENSE file in the root directory of this
## source tree. An additional grant of patent rights can be found in the
## PATENTS file in the same directory.
##

cmake_minimum_required(VERSION 2.8.12)

project(JSObjects)

find_package(BISON)
find_package(FLEX)

FLEX_TARGET(JSON_SCANNER
            ${JSObjects_SOURCE_DIR}/Sources/libjson/tokenizer.l
            ${JSObjects_BINARY_DIR}/tokenizer.c)
BISON_TARGET(JSON_PARSER
             ${JSObjects_SOURCE_DIR}/Sources/libjson/parser.y
             ${JSObjects_BINARY_DIR}/parser.c
             COMPILE_FLAGS --no-lines)
ADD_FLEX_BISON_DEPENDENCY(JSON_SCANNER JSON_PARSER)

add_library(jsobjects STATIC
            ${BISON_JSON_PARSER_OUTPUTS}
            ${FLEX_JSON_SCANNER_OUTPUTS}
            Sources/libjson/json.c
            Sources/libjson/qstring.c
            Sources/JSObjects.cpp
            )

target_include_directories(jsobjects
                           PUBLIC ${JSObjects_SOURCE_DIR}/Headers
                           PRIVATE ${JSObjects_SOURCE_DIR}/Sources/libjson
                           PRIVATE ${JSObjects_BINARY_DIR}
                           )

set(COMMON_FLAGS "-D_XOPEN_SOURCE=500 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 ${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ${COMMON_FLAGS}")
