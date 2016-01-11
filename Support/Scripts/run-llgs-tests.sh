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

[ "$(uname)" == "Linux" ] || die "The lldb-gdbserver test suite requires a Linux host environment."
[ -x "./ds2" ]            || die "Unable to find a ds2 binary in the current directory."

lldb_path="/tmp/lldb"
git_clone "$LLDB_REPO" "$lldb_path"   "$UPSTREAM_BRANCH"

for p in "Hacks-to-use-ds2-instead-of-llgs"; do
  echo "Applying $p.patch"
  patch -d "$lldb_path" -p1 <"$(dirname "$0")/../Testing/$p.patch"
done

python_base="/usr/lib/x86_64-linux-gnu"
export PYTHONPATH="$python_base/python2.7/site-packages"
sudo cp -r /usr/lib/llvm-3.7/lib/python2.7 "$python_base"
sudo ln -s "$python_base/liblldb-3.7.so" "$python_base/liblldb.so"
cd "$python_base/python2.7/site-packages/lldb"
for path in $( ls *.so* ); do
  new_link="$(readlink "$path" | sed 's/x86_64-linux-gnu//g')"
  sudo rm "$path"
  sudo ln -s "$new_link" "$path"
done

cd "$lldb_path/test"
lldb_exe="$(which lldb-3.7)"
LLDB_DEBUGSERVER_PATH="/tmp/ds2" python2.7 dotest.py -q --arch=x86_64 --executable "$lldb_exe" -u CXXFLAGS -u CFLAGS -C /usr/bin/cc -p TestGdbRemote -m
