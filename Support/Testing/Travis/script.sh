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

# Get a recent cmake from cmake.org. All packages for Ubuntu 12.04 are too old.
cd /tmp
cmake_package="cmake-3.4.0-Linux-x86_64"

if [ ! -e "$cmake_package.tar.gz" ] ; then
  wget --no-check-certificate "https://cmake.org/files/v3.4/$cmake_package.tar.gz"
fi
tar -xf "$cmake_package.tar.gz"

export PATH="$PWD/$cmake_package/bin:$PATH"
cd "$OLDPWD"

cd "$(git rev-parse --show-toplevel)"

cformat="clang-format-3.7"

check_dirty() {
  dirty=($(git status -s | awk '{ print $2 }'))
  if [[ "${#dirty[@]}" -eq 0 ]]; then
    echo "$1"
    exit 0
  else
    echo "$2"
    for f in "${dirty[@]}"; do
      echo "dirty: $f"
    done
    exit 1
  fi
}

if [[ "$TARGET" = "Style" ]]; then
  for d in Sources Headers; do
    find "$d" -type f -exec "$cformat" -i -style=LLVM {} \;
  done
  check_dirty "Coding style correct." "Coding style errors."
fi

if [[ "$TARGET" = "Registers" ]]; then
  CLANG_FORMAT="$cformat" CC="gcc-4.8" CXX="g++-4.8" "./Support/Scripts/generate-reg-descriptors.sh"
  check_dirty "Generated sources up to date." "Generated sources out of date."
fi

mkdir -p build && cd build

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

if [[ -n "${LLDB_TESTS-}" ]]; then
  ../Support/Scripts/run-llgs-tests.sh
fi
