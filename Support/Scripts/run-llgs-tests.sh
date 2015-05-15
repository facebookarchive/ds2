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

# This script runs the lldb-gdbserver test suite found in the LLDB repository
# against ds2. It requires a few hacks in the testing infra to adapt to the
# differences between ds2 and lldb-gdbserver.

LLVM_REPO="https://github.com/llvm-mirror/llvm.git"
CLANG_REPO="https://github.com/llvm-mirror/clang.git"
LLDB_REPO="https://github.com/llvm-mirror/lldb.git"

source "$(dirname "$0")/common.sh"

[ "$(uname)" == "Linux" ] || die "The lldb-gdbserver test suite requires a Linux host environment."
[ -x "./ds2" ]            || die "Unable to find a ds2 binary in the current directory."

git_clone "$LLVM_REPO"  llvm
git_clone "$CLANG_REPO" llvm/tools/clang
git_clone "$LLDB_REPO"  llvm/tools/lldb

for p in "Hacks-to-use-ds2-instead-of-llgs" "Use-gcc-instead-of-clang-for-testing"; do
  echo "Applying $p.patch"
  patch -d "llvm/tools/lldb" -p1 <"$(dirname "$0")/../Testing/$p.patch"
done

rm -rf llvm/build
mkdir -p llvm/build
cd llvm/build
cmake -DCMAKE_C_COMPILER="gcc-4.8" -DCMAKE_CXX_COMPILER="g++-4.8" -DLLDB_TEST_USER_ARGS="-p;TestGdbRemote" ..
LLDB_DEBUGSERVER_PATH="$(pwd)/../../ds2" make check-lldb-single
