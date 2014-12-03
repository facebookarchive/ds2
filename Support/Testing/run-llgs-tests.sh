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

LLVM_REPO="http://llvm.org/git/llvm.git"
CLANG_REPO="http://llvm.org/git/clang.git"
LLDB_REPO="http://llvm.org/git/lldb.git"

set -eu

die() {
    echo "error:" "$@" >&2
    exit 1
}

if [ "$(uname)" != "Linux" ]; then
    die "The lldb-gdbserver test suite requires a Linux host environment."
fi

if [ ! -x "./ds2" ]; then
    die "Unable to find a ds2 binary in the current directory."
fi

clone() {
    if [ ! -e "$2" ]; then
        git clone --depth 1 "$1" "$2"
    elif [ -d "$2/.git" ]; then
        cd "$2"
        git reset --hard
        git pull --rebase
        cd "$OLDPWD"
    else
        die "'$2' exists and is not a git repository."
    fi
}

clone "$LLVM_REPO" llvm
clone "$CLANG_REPO" llvm/tools/clang
clone "$LLDB_REPO" llvm/tools/lldb

for p in "Hacks-to-use-ds2-instead-of-llgs" "Fixes-to-dotest.py"; do
    echo "Applying $p.patch"
    patch -d "llvm/tools/lldb" -p1 <"$(dirname "$0")/$p.patch"
done

rm -rf llvm/build
mkdir -p llvm/build
cd llvm/build
cmake -DCMAKE_C_COMPILER="gcc-4.8" -DCMAKE_CXX_COMPILER="g++-4.8" -DLLDB_TEST_USER_ARGS="-p;TestGdbRemote" ..
LLDB_DEBUGSERVER_PATH="$(pwd)/../../ds2" make check-lldb-single
