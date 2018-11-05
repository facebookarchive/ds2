##
## Copyright (c) 2014-present, Facebook, Inc.
## All rights reserved.
##
## This source code is licensed under the University of Illinois/NCSA Open
## Source License found in the LICENSE file in the root directory of this
## source tree. An additional grant of patent rights can be found in the
## PATENTS file in the same directory.
##

cmake_minimum_required(VERSION 3.3)

project(JSObjects)

find_package(BISON)
find_package(FLEX)

FLEX_TARGET(JSON_SCANNER
            ${JSObjects_SOURCE_DIR}/Sources/libjson/tokenizer.l
            ${JSObjects_BINARY_DIR}/tokenizer.c
            COMPILE_FLAGS "--header-file=${JSObjects_BINARY_DIR}/tokenizer.h \
                           --nounistd --batch --never-interactive")
BISON_TARGET(JSON_PARSER
             ${JSObjects_SOURCE_DIR}/Sources/libjson/parser.y
             ${JSObjects_BINARY_DIR}/parser.c
             COMPILE_FLAGS "--no-lines")
ADD_FLEX_BISON_DEPENDENCY(JSON_SCANNER JSON_PARSER)

set(JSOBJECTS_SOURCES
    ${BISON_JSON_PARSER_OUTPUTS}
    ${FLEX_JSON_SCANNER_OUTPUTS}
    Sources/JSObjects.cpp
    Sources/libjson/json.c
    Sources/libjson/qstring.c
    )

add_library(jsobjects STATIC ${JSOBJECTS_SOURCES})
target_include_directories(jsobjects
                           PUBLIC ${JSObjects_SOURCE_DIR}/Headers
                           PRIVATE ${JSObjects_SOURCE_DIR}/Sources/libjson
                           PRIVATE ${JSObjects_BINARY_DIR})
set_property(TARGET jsobjects PROPERTY C_STANDARD 99)
set_property(TARGET jsobjects PROPERTY C_EXTENSIONS OFF)
set_property(TARGET jsobjects PROPERTY CXX_STANDARD 11)
set_property(TARGET jsobjects PROPERTY CXX_EXTENSIONS OFF)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR
    (CMAKE_CXX_COMPILER_ID MATCHES "Clang" 
    AND NOT CMAKE_CXX_SIMULATE_ID MATCHES "MSVC"))
  target_compile_options(jsobjects PRIVATE -Wall -Wextra -Werror)
  target_compile_options(jsobjects PRIVATE -Wno-unused-parameter
                                           -Wno-unused-function
                                           -Wno-sign-compare)
  target_compile_options(
      jsobjects PRIVATE $<$<COMPILE_LANGUAGE:C>:-D_POSIX_C_SOURCE=200809L>)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  target_compile_options(jsobjects PRIVATE /W3 /DSTRICT /wd4996)
  target_compile_options(jsobjects PRIVATE /MP)
endif ()
