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

set -eu

cmake_package="cmake-3.4.0-Linux-x86_64"
cformat="${CLANG_FORMAT_PATH-clang-format-3.8}"
top="$(git rev-parse --show-toplevel)"

source "$top/Support/Scripts/common.sh"

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

do_build() {
  local target="$1"
  local compiler="$2"
  local do_test="$3"
  local cmake_options=()

  if [[ "${compiler}" = "clang" ]]; then
    cmake_options+=(-DCMAKE_TOOLCHAIN_FILE="../Support/CMake/Toolchain-${target}-Clang.cmake")
  else
    cmake_options+=(-DCMAKE_TOOLCHAIN_FILE="../Support/CMake/Toolchain-${target}.cmake")
  fi

  cmake_options+=(-DCMAKE_BUILD_TYPE="Debug")

  # If we're at the root of the repository, create a build directory and go
  # there; otherwise, assume that we're already in the build directory the user
  # wants to use.
  cd .
  if same_dir "$PWD" "$top"; then
    mkdir -p "build-${target}"
    cd "build-${target}"
  fi

  cmake "${cmake_options[@]}" "$top"
  make -j$(num_cpus)
  if $do_test; then
    TARGET="${target}" LLDB_TESTS="all" "$top/Support/Scripts/run-lldb-tests.sh"
  fi

  cd "$OLDPWD"
}

compiler="${CC-}"

if [[ "${ANDROID_BUILDS-}" = "1" ]]; then
  failed=false
  for a in ARM ARM64 X86 X86_64; do
    if ! do_build "Android-${a}" "$compiler" false; then
      failed=true
    fi
  done
  $failed && exit 1
else
  case "$TRAVIS_OS_NAME" in
    linux) target_os="Linux"; do_test=true;;
    osx) target_os="Darwin"; do_test=false;;
  esac
  do_build "${target_os}-${ARCH}" "$compiler" "$do_test"
fi

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

# # Go to the root of the repo to check style and register files.
# cd "$top"
#
# if [[ "$TARGET" = "Style" ]]; then
#   for d in Sources Headers; do
#     find "$d" -type f -exec "$cformat" -i -sort-includes -style=LLVM {} \;
#   done
#   check_dirty "Coding style correct." "Coding style errors."
# fi
#
# if [[ "$TARGET" = "Registers" ]]; then
#   CLANG_FORMAT="$cformat" CC="gcc-5" CXX="g++-5" "./Support/Scripts/generate-reg-descriptors.sh"
#   check_dirty "Generated sources up to date." "Generated sources out of date."
# fi
#
# # Go back to the build tree where we started.
# cd "$OLDPWD"
