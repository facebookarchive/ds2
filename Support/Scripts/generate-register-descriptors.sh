#!/usr/bin/env bash
## Copyright (c) Meta Platforms, Inc. and affiliates.
##
## This source code is licensed under the Apache License v2.0 with LLVM
## Exceptions found in the LICENSE file in the root directory of this
## source tree.

set -eu

architectures=("X86" "X86_64" "ARM" "ARM64")

while test $# -gt 0; do
  case "$1" in
    --clang-format) shift
      CLANG_FORMAT="$1";;
    *) die "Unknown option \`$1'.";;
  esac
  shift
done

cformat="${CLANG_FORMAT-clang-format}"

repo_root="$(cd "$(dirname "$0")" && git rev-parse --show-toplevel)"
cd "$repo_root"

build_dir="Tools/RegsGen2/build"
if [ ! -x "$build_dir/regsgen2" ]; then
  echo "regsgen2 not found, building."
  mkdir -p "$build_dir"
  cd "$build_dir"
  cmake -DCMAKE_TOOLCHAIN_FILE="${repo_root}/Support/CMake/Toolchain-Linux-X86_64.cmake" ..
  make regsgen2
fi

cd "$repo_root"

for a in "${architectures[@]}"; do
  header_path="Headers/DebugServer2/Architecture/$a/RegistersDescriptors.h"
  source_path="Sources/Architecture/$a/RegistersDescriptors.cpp"

  "$build_dir/regsgen2"                             \
    -a "linux"                                      \
    -I "DebugServer2/Architecture/RegisterLayout.h" \
    -h -o "$header_path"                            \
    -f "Definitions/$a.json"

  "$build_dir/regsgen2"                                       \
    -a "linux"                                                \
    -I "DebugServer2/Architecture/$a/RegistersDescriptors.h"  \
    -c -o "$source_path"                                      \
    -f "Definitions/$a.json"

  "$cformat" -i -style=LLVM "$header_path" "$source_path"
done
