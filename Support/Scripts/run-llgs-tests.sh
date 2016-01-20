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

[ "$(uname)" == "Linux" ] || die "The lldb-gdbserver test suite requires a Linux host environment."
[ -x "$top/ds2" ]         || die "Unable to find a ds2 binary in the current directory."

lldb_path="$top/lldb"
git_clone "$LLDB_REPO" "$lldb_path"   "$UPSTREAM_BRANCH"

for p in $top/../Support/Testing/*.patch ; do
  echo "Applying $p"
  patch -d "$lldb_path" -p1 < "$p"
done

python_base="$top/lib"
export PYTHONPATH="$python_base/python2.7/site-packages"

# Sync lldb libs to local build dir
rsync -a /usr/lib/x86_64-linux-gnu/       "$python_base"
rsync -a /usr/lib/llvm-3.7/lib/python2.7  "$python_base"
rsync -a "$python_base/liblldb-3.7.so"    "$python_base/liblldb.so"

# Fix broken python lldb symlinks
cd "$PYTHONPATH/lldb"
for path in $( ls *.so* ); do
  new_link="$(readlink "$path" | sed 's/x86_64-linux-gnu//g')"
  rsync -a "$new_link" "$path"
done

cd "$lldb_path/test"
lldb_exe="$(which lldb-3.7)"

args="-q --arch=x86_64 --executable "$lldb_exe" -u CXXFLAGS -u CFLAGS -C /usr/bin/cc -m"
if [ "$LLDB_TESTS" != "all" ]; then
  args="$args -p $LLDB_TESTS"
fi

for attempt in 0 1; do
  if [ $attempt -ne 0 ]; then
    echo "Failed test suite: Test$LLDB_TESTS, retrying"
  fi

  if LLDB_DEBUGSERVER_PATH="$top/ds2" python2.7 dotest.py $args; then
    break
  fi
done
