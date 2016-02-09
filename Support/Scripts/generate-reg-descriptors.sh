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

architectures=("X86" "X86_64" "ARM" "ARM64")

cformat="${CLANG_FORMAT-clang-format}"

repo_root="$(cd "$(dirname "$0")" && git rev-parse --show-toplevel)"
cd "$repo_root"

build_dir="Tools/RegsGen2/build"
if [ ! -x "$build_dir/regsgen2" ]; then
  echo "regsgen2 not found, building."
  mkdir -p "$build_dir"
  cd "$build_dir"
  cmake ..
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
