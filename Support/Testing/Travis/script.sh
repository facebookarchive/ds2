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

cmake_package="cmake-3.4.0-Linux-x86_64"
cformat="clang-format-3.7"
top="$(git rev-parse --show-toplevel)"

source "$top/Support/Scripts/common.sh"

# If we're at the root of the repository, create a build directory and go
# there; otherwise, assume that we're already in the build directory the user
# wants to use.
if same_dir "$PWD" "$top"; then
  mkdir -p build && cd build
fi

# Get a recent cmake from cmake.org; packages for Ubuntu are too old.
if grep -q "Ubuntu" "/etc/issue" && [ ! -d "/tmp/$cmake_package/bin" ]; then
  cd /tmp

  if [ ! -e "$cmake_package.tar.gz" ] ; then
    wget --no-check-certificate "https://cmake.org/files/v3.4/$cmake_package.tar.gz"
  fi

  tar -xf "$cmake_package.tar.gz"

  cd "$OLDPWD"
fi

export PATH="/tmp/$cmake_package/bin:$PATH"

ninja_version="v1.6.0"
ninja_dir="ninja-$ninja_version"

if [ -s "/etc/centos-release" ] && [ ! -d "/tmp/$ninja_dir" ]; then
  cd /tmp

  mkdir -p "$ninja_dir"

  if [ ! -e "ninja-linux.zip" ]; then
    wget https://github.com/ninja-build/ninja/releases/download/$ninja_version/ninja-linux.zip
  fi

  unzip ninja-linux.zip -d "$ninja_dir"

  cd "$OLDPWD"
fi

export PATH="/tmp/$ninja_dir:$PATH"

ninja_version="v1.6.0"
ninja_dir="ninja-$ninja_version"

if [ -s "/etc/centos-release" ] && [ ! -d "/tmp/$ninja_dir" ]; then
  cd /tmp  

  mkdir -p "$ninja_dir"

  if [ ! -e "ninja-linux.zip" ]; then
    wget https://github.com/ninja-build/ninja/releases/download/$ninja_version/ninja-linux.zip 
  fi

  unzip ninja-linux.zip -d "$ninja_dir"

  cd "$OLDPWD"
fi

export PATH="/tmp/$ninja_dir:$PATH"

# Go to the root of the repo to check style and register files.
cd "$top"

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

# Go back to the build tree where we started.
cd "$OLDPWD"

# CentOS uses different compiler names than Ubuntu, so we can only use the
# toolchain files on Ubuntu In addition, clang-3.7 is also not available for
# CentOS
if grep -q "Ubuntu" "/etc/issue"; then
  if [[ "${CLANG-}" = "1" ]]; then
    cmake_options=(-DCMAKE_TOOLCHAIN_FILE="../Support/CMake/Toolchain-${TARGET}-Clang.cmake")
  else
    cmake_options=(-DCMAKE_TOOLCHAIN_FILE="../Support/CMake/Toolchain-${TARGET}.cmake")
  fi
fi

if [[ "${RELEASE-}" = "1" ]]; then
  cmake_options+=(-DCMAKE_BUILD_TYPE="Release")
else
  cmake_options+=(-DCMAKE_BUILD_TYPE="Debug")
fi

cmake "${cmake_options[@]}" "$top"
make -j$(num_cpus)

if [[ -n "${LLDB_TESTS-}" ]]; then
  "$top/Support/Scripts/run-lldb-tests.sh"
fi
