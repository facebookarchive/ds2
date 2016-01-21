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

LLDB_REPO="https://github.com/llvm-mirror/lldb.git"
UPSTREAM_BRANCH="release_37"

source "$(dirname "$0")/common.sh"

top="$(pwd)"
testPath="$top/../Support/Testing"

[ "$(uname)" == "Linux" ] || die "The lldb-gdbserver test suite requires a Linux host environment."
[ -x "$top/ds2" ]         || die "Unable to find a ds2 binary in the current directory."

lldb_path="$top/lldb"
git_clone "$LLDB_REPO" "$lldb_path"   "$UPSTREAM_BRANCH"

for p in $testPath/Patches/*.patch ; do
  echo "Applying $p"
  patch -d "$lldb_path" -p1 < "$p"
done

python_base="$top/lib"

export LD_LIBRARY_PATH=$python_base
export PYTHONPATH="$python_base/python2.7/site-packages"

# Sync lldb libs to local build dir
rsync -a /usr/lib/x86_64-linux-gnu/       "$python_base"
rsync -a /usr/lib/llvm-3.7/lib/python2.7/ "$python_base/python2.7"
rsync -a "$python_base/liblldb-3.7.so"    "$python_base/liblldb.so"

# Fix broken python lldb symlinks
cd "$PYTHONPATH/lldb"
for path in $( ls *.so* ); do
  new_link="$(readlink "$path" | sed 's,x86_64-linux-gnu/,,g')"
  rm "$path"
  ln -s "$new_link" "$path"
done

cd "$lldb_path/test"
lldb_exe="$(which lldb-3.7)"

# If LLDB_TESTS = "all", we run the full lldb test suite
# If LLDB_TESTS is a number in [1:10], we run a subset of lldb tests from testConfig.txt
# If LLDB_TESTS is any other value, we run the set of lldb tests which regex matches the value
args="-q --arch=x86_64 --executable "$lldb_exe" -u CXXFLAGS -u CFLAGS -C /usr/bin/cc -m"
if [ "$LLDB_TESTS" != "all" ]; then
  found=false
  while read line
  do
    if $found ; then
      args="$args -p $line"
      break
    elif [ "$line" == "*$LLDB_TESTS*" ]; then
      found=true
    fi
  done < "$testPath/Config/testConfig.txt"

  if ! $found ; then
    args="$args -p $LLDB_TESTS"
  fi
fi

LLDB_DEBUGSERVER_PATH="$top/ds2" python2.7 dotest.py $args
