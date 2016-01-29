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

# This script runs the full lldb test suite found in the LLDB repository
# against ds2. It requires a few hacks in the testing infra to disable
# broken unit tests.

REPO_BASE="https://github.com/llvm-mirror"
UPSTREAM_BRANCH="release_37"

top="$(git rev-parse --show-toplevel)"
build_dir="$PWD"

source "$top/Support/Scripts/common.sh"

[ "$(uname)" == "Linux" ] || die "The lldb test suite requires a Linux host environment."
[ -x "$build_dir/ds2" ]   || die "Unable to find a ds2 binary in the current directory."

if [ -s "/etc/centos-release" ]; then
  llvm_path="$build_dir/llvm"
  git_clone "$REPO_BASE/llvm.git"  "$llvm_path"             "$UPSTREAM_BRANCH"
  git_clone "$REPO_BASE/lldb.git"  "$llvm_path/tools/lldb"  "$UPSTREAM_BRANCH"
  git_clone "$REPO_BASE/clang.git" "$llvm_path/tools/clang" "$UPSTREAM_BRANCH"

  mkdir -p "$llvm_path/build"
  cd "$llvm_path/build"
  cmake ..
  make -j$(num_cpus)

  export PYTHONPATH="$llvm_path/build/lib64/python2.7/site-packages"

  patch -d "$llvm_path/tools/lldb" -p1 <"$top/Support/Testing/Pythonpath-hack.patch"

  lldb_exe="$llvm_path/build/bin/lldb"
  lldb_path="$llvm_path/tools/lldb"
  cc_exe="$(which gcc)"
elif grep -q "Ubuntu" "/etc/issue"; then
  lldb_path="$build_dir/lldb"
  git_clone "$REPO_BASE/lldb.git" "$lldb_path" "$UPSTREAM_BRANCH"

  python_base="$build_dir/lib"

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

  lldb_exe="$(which lldb-3.7)"
  cc_exe="$(which gcc-4.8)"
else
  die "Testing is only supported on CentOS and Ubuntu."
fi

testPath="$top/Support/Testing"
for p in $testPath/Patches/*.patch ; do
  echo "Applying $p"
  patch -d "$lldb_path" -p1 <"$p"
done

cd "$lldb_path/test"
args="-q --executable "$lldb_exe" -u CXXFLAGS -u CFLAGS -C $cc_exe -m"

if [ -n "${TARGET-}" ]; then
  if [[ "${TARGET}" = "Linux-X86_64" ]]; then
    args="$args --arch=x86_64"
  elif [[ "${TARGET}" = "Linux-X86" ]]; then
    args="$args --arch=i386"
  fi
else
  # If this is a developer run (not running on Travis with a $TARGET), run all
  # tests by default.
  LLDB_TESTS="${LLDB_TESTS-all}"

  if [[ "$(uname -m)" = "x86_64" ]]; then
    args="$args --arch=x86_64"
  elif [[ "$(uname -m)" =~ "i[3-6]86" ]]; then
    args="$args --arch=i386"
  fi
fi

if [ "$LLDB_TESTS" != "all" ]; then
  LLDB_DEBUGSERVER_PATH="$build_dir/ds2" python2.7 dotest.py $args -p $LLDB_TESTS
else
  LLDB_TEST_TIMEOUT=45m LLDB_DEBUGSERVER_PATH="$build_dir/ds2" python2.7 dosep.py -o "$args"
fi
