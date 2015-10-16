#!/usr/bin/env bash
##
## Copyright (c) 2014, Facebook, Inc.
## All rights reserved.
##
## This source code is licensed under the University of Illinois/NCSA Open
## Source License found in the LICENSE file in the root directory of this
## source tree. An additional grant of patent rights can be found in the
## PATENTS file in the same directory.
##

set -eu

cd "$(git rev-parse --show-toplevel)"

cformat="clang-format-3.6"

if [[ "$TARGET" = "Style" ]]; then
  CLANG_FORMAT="$cformat" "./Support/Scripts/check-style.sh" {Sources,Headers,Main}
  exit
fi

if [[ "$TARGET" = "Registers" ]]; then
  CLANG_FORMAT="$cformat" CC="gcc-4.8" CXX="g++-4.8" "./Support/Scripts/generate-reg-descriptors.sh"
  dirty=($(git status -s | awk '{ print $2 }'))
  if [[ "${#dirty[@]}" -eq 0 ]]; then
    echo "Generated sources up to date."
    exit 0
  else
    echo "Generated sources out of date:"
    for f in "${dirty[@]}"; do
      echo "Out of date: $f"
    done
    exit 1
fi

mkdir build && cd build

if [[ "${CLANG-}" = "1" ]]; then
  cmake_options=(-DCMAKE_TOOLCHAIN_FILE="../Support/CMake/Toolchain-${TARGET}-Clang.cmake")
else
  cmake_options=(-DCMAKE_TOOLCHAIN_FILE="../Support/CMake/Toolchain-${TARGET}.cmake")
fi

if [[ "${RELEASE-}" = "1" ]]; then
  cmake_options+=(-DCMAKE_BUILD_TYPE="Release")
fi

cmake "${cmake_options[@]}" ..
make

if [[ -n "${LLGS_TESTS-}" ]]; then
  ../Support/Scripts/run-llgs-tests.sh
fi
