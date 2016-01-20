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

tests=(GdbRemote StubReverseConnect LldbGdbServer Help Settings)

LLDB_REPO="https://github.com/llvm-mirror/lldb.git"
UPSTREAM_BRANCH="release_37"

source "$(dirname "$0")/common.sh"

top="$(pwd)"

[ "$(uname)" == "Linux" ] || die "The lldb-gdbserver test suite requires a Linux host environment."
[ -x "./ds2" ]            || die "Unable to find a ds2 binary in the current directory."

lldb_path="$top/lldb"
git_clone "$LLDB_REPO" "$lldb_path"   "$UPSTREAM_BRANCH"

cd "$(dirname "$0")/../Testing"
for p in $( ls *.patch ) ; do
  echo "Applying $p"
  patch -d "$lldb_path" -p1 < "$p"
done

python_base="$top/lib"
export PYTHONPATH="$python_base/python2.7/site-packages"

if [ ! -e "$python_base" ] ; then
  mkdir -p "$python_base"
  cp -r /usr/lib/x86_64-linux-gnu/* "$python_base"
fi

if [ ! -e "$PYTHONPATH" ] ; then
  cp -r /usr/lib/llvm-3.7/lib/python2.7 "$python_base"
fi

if [ ! -e "$python_base/liblldb.so" ] ; then
  ln -s "$python_base/liblldb-3.7.so" "$python_base/liblldb.so"
fi

cd "$python_base/python2.7/site-packages/lldb"
for path in $( ls *.so* ); do
  new_link="$(readlink "$path" | sed 's/x86_64-linux-gnu//g')"
  rm "$path"
  ln -s "$new_link" "$path"
done

cd "$lldb_path/test"
lldb_exe="$(which lldb-3.7)"

args="-q --arch=x86_64 --executable "$lldb_exe" -u CXXFLAGS -u CFLAGS -C /usr/bin/cc -m"
for test in ${tests[@]}; do
  for attempt in 0 1; do
    if [ $attempt -ne 0 ]; then
      echo "Failed test suite: Test$test, retrying"
    fi

    if LLDB_DEBUGSERVER_PATH="$top/ds2" python2.7 dotest.py $args -p "$test"; then
      break
    fi
  done
done
