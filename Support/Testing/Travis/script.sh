#!/usr/bin/env bash
##
## Copyright (c) 2014-present, Facebook, Inc.
## All rights reserved.
##
## This source code is licensed under the University of Illinois/NCSA Open
## Source License found in the LICENSE file in the root directory of this
## source tree. An additional grant of patent rights can be found in the
## PATENTS file in the same directory.
##

top="$(git rev-parse --show-toplevel)"
source "$top/Support/Scripts/common.sh"

cformat="clang-format-5.0"

# If we're at the root of the repository, create a build directory and go
# there; otherwise, assume that we're already in the build directory the user
# wants to use.
if same_dir "$PWD" "$top"; then
  mkdir -p build && cd build
fi

# Get a recent cmake from cmake.org; packages for Ubuntu are too old.
cmake_version="3.7"
cmake_package="cmake-${cmake_version}.0-Linux-x86_64"
if [ "$(linux_distribution)" == "ubuntu" ] && [ ! -d "/tmp/$cmake_package/bin" ]; then
  cd /tmp

  if [ ! -e "$cmake_package.tar.gz" ] ; then
    wget "https://cmake.org/files/v${cmake_version}/${cmake_package}.tar.gz"
  fi

  tar -xf "${cmake_package}.tar.gz"
  rm -f "${cmake_package}.tar.gz"

  cd "$OLDPWD"
fi
export PATH="/tmp/$cmake_package/bin:$PATH"

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
    git --no-pager diff
    exit 1
  fi
}

if [[ "$TARGET" = "Style" ]]; then
  for d in Sources Headers; do
    find "$d" -type f -exec "$cformat" -i -style=file {} \;
  done
  check_dirty "Coding style correct." "Coding style errors."
fi

if [[ "$TARGET" = "Registers" ]]; then
  CLANG_FORMAT="$cformat" ./Support/Scripts/generate-register-descriptors.sh
  check_dirty "Generated sources up to date." "Generated sources out of date."
fi

if [[ "$TARGET" = "Documentation" ]]; then
  ./Support/Scripts/generate-documentation.sh
  exit
fi

# Go back to the build tree where we started.
cd "$OLDPWD"

cmake_options=(-G Ninja)

# Except for the Android toolchain that we install ourselves, CentOS uses
# different compiler names than Ubuntu, so we can only use the toolchain files
# on Ubuntu. In addition, clang-5.0 is also not available for CentOS
if [ "$(linux_distribution)" != "centos" ] || [[ "$TARGET" == Android-* ]]; then
  target_name="${TARGET}"

  if [[ "${CLANG-}" = "1" ]]; then
    target_name="${target_name}-Clang"
  fi

  cmake_options+=(-DCMAKE_TOOLCHAIN_FILE="../Support/CMake/Toolchain-${target_name}.cmake")
fi

if [[ "${RELEASE-}" = "1" ]]; then
  cmake_options+=(-DCMAKE_BUILD_TYPE="Release")
else
  cmake_options+=(-DCMAKE_BUILD_TYPE="Debug")
fi

if [[ "${COVERAGE-}" = "1" ]]; then
  cmake_options+=(-DCOVERAGE="1")
fi

if [[ "${CLANG-}" = "1" ]] && [[ "$TARGET" = "Linux-X86_64" ]]; then
  cmake_options+=(-DSANITIZER="asan")
fi

cmake "${cmake_options[@]}" "$top"
ninja

if [[ -n "${LLDB_TESTS-}" ]]; then
  "$top/Support/Scripts/run-lldb-tests.sh"
fi

if [[ -n "${GDB_TESTS-}" ]]; then
  "$top/Support/Scripts/run-gdb-tests.sh"
fi
