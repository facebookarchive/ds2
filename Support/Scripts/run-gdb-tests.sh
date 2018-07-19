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

# This script runs parts of the gdb test suite found in the gdb repository
# against ds2. It requires a few hacks in the testing infra to disable
# broken unit tests.

top="$(git rev-parse --show-toplevel)"
build_dir="$PWD"

[ "$(uname)" == "Linux" ] || die "The gdb test suite requires a Linux host environment."
[ -x "$build_dir/ds2" ]   || die "Unable to find a ds2 binary in the current directory."

while test $# -gt 0; do
  case "$1" in
    --gdb-tests) shift
      GDB_TESTS="$1";;
    *) die "Unknown option \`$1'.";;
  esac
  shift
done

source "$top/Support/Scripts/common.sh"

gdb_ftp_path="https://ftp.gnu.org/gnu/gdb"
gdb_basename="gdb-7.11.1"
gdb_src_path="$build_dir/$gdb_basename"

wget "$gdb_ftp_path/$gdb_basename.tar.gz"
tar -zxf "$gdb_basename.tar.gz"
rm -f "${gdb_basename}.tar.gz"

testingPath="$top/Support/Testing"
for p in $testingPath/Patches/gdb/*.patch; do
  echo "Applying $p"
  patch -d "$gdb_src_path" -p1 <"$p"
done

cd "$gdb_src_path/gdb/testsuite"
ds2_test_dir="ds2.tests"
mkdir -p "$ds2_test_dir"

IFS=';' read -ra TEST <<< "$GDB_TESTS"
for test_name in "${TEST[@]}"; do
  cp $test_name.exp $ds2_test_dir
  cp $test_name*.c $ds2_test_dir
done

./configure

gdb_exe="$(which gdb)"
ds2_wrapper="$top/Support/Scripts/ds2-gdb-wrapper.sh"
make check RUNTESTFLAGS="GDB=$gdb_exe GDBSERVER=$ds2_wrapper -target_board=native-gdbserver" TESTS="$ds2_test_dir/*.exp"
