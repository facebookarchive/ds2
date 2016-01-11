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

source "$(dirname "$0")/common.sh"

[ "$(uname)" == "Linux" ] || die "The lldb-gdbserver test suite requires a Linux host environment."
[ -x "./ds2" ]            || die "Unable to find a ds2 binary in the current directory."

lldb_path="/tmp/llvm/tools/lldb"

for p in "Hacks-to-use-ds2-instead-of-llgs"; do
  echo "Applying $p.patch"
#  patch -d "llvm/tools/lldb" -p1 <"$(dirname "$0")/../Testing/$p.patch"
  patch -d "$lldb_path" -p1 <"$(dirname "$0")/../Testing/$p.patch"
done



cd "$lldb_path/test"
#cmake -DCMAKE_C_COMPILER="gcc-4.8" -DCMAKE_CXX_COMPILER="g++-4.8" -DLLDB_TEST_USER_ARGS="-p;TestGdbRemote" ..
LLDB_DEBUGSERVER_PATH="/tmp/ds2" timeout -s QUIT 4m python2.7 dotest.py -q --arch=x86_64 --executable /tmp/llvm/build/bin/lldb -s /tmp/llvm/build/lldb-test-traces -u CXXFLAGS -u CFLAGS -C /usr/bin/cc -p TestGdbRemote
