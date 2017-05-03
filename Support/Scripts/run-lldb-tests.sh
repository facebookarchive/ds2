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

# This script runs the full lldb test suite found in the LLDB repository
# against ds2. It requires a few hacks in the testing infra to disable
# broken unit tests.

REPO_BASE="https://github.com/llvm-mirror"
UPSTREAM_BRANCH="release_39"

top="$(git rev-parse --show-toplevel)"
build_dir="$PWD"

source "$top/Support/Scripts/common.sh"

[ "$(uname)" == "Linux" ] || die "The lldb test suite requires a Linux host environment."
[ -x "$build_dir/ds2" ]   || die "Unable to find a ds2 binary in the current directory."

opt_fast=false
opt_log=false
opt_strace=false

for o in "$@"; do
  case "$o" in
    --fast) opt_fast=true;;
    --log) opt_log=true;;
    --strace) opt_strace=true;;
    *) die "Unknown option \`$o'.";;
  esac
done

if [[ "${TARGET-}" = Android-* ]]; then
  platform_name="android"
else
  platform_name="linux"
fi

cherry_pick_patches() {
  cd "$1"

  # Disabled and enabled tests
  local testingPath="$top/Support/Testing"
  patch -d "$lldb_path" -p1 <"$testingPath/Patches/lldb-test-blacklist.patch"
  patch -d "$lldb_path" -p1 <"$testingPath/Patches/lldb-blacklist-update.patch"

  if [[ "$platform_name" = "android" ]]; then
    patch -d "$lldb_path" -p1 <"$testingPath/Patches/android-search-paths.patch"
  fi

  patch -d "$lldb_path" -p1 <"$testingPath/Patches/memory-region-info.patch"

  cd "$OLDPWD"
}

if [ "$(linux_distribution)" == "centos" ]; then
  llvm_path="$build_dir/llvm"
  llvm_build="$llvm_path/build"
  lldb_path="$llvm_path/tools/lldb"
  lldb_exe="$llvm_build/bin/lldb"
  cc_exe="$(which gcc)"

  if ! $opt_fast; then
    git_clone "$REPO_BASE/llvm.git"  "$llvm_path"             "$UPSTREAM_BRANCH"
    git_clone "$REPO_BASE/lldb.git"  "$llvm_path/tools/lldb"  "$UPSTREAM_BRANCH"
    git_clone "$REPO_BASE/clang.git" "$llvm_path/tools/clang" "$UPSTREAM_BRANCH"

    cherry_pick_patches "$llvm_path/tools/lldb"

    if [ -z "${LLDB_EXE-}" ] ; then
      mkdir -p "$llvm_build"
      cd "$llvm_build"
      cmake -G Ninja -DLLVM_LIBDIR_SUFFIX=64 ..
      ninja lldb
    fi
  fi
elif [ "$(linux_distribution)" == "ubuntu" ]; then
  lldb_path="$build_dir/lldb"
  lldb_exe="$(which lldb-3.9)"

  case "${TARGET}" in
    "Android-ARM") cc_exe="/tmp/aosp-toolchain/arm/bin/arm-linux-androideabi-gcc";;
    "Android-X86") cc_exe="/tmp/aosp-toolchain/x86/bin/i686-linux-android-gcc";;
    *)             cc_exe="$(which gcc-5)"
  esac

  python_base="$build_dir/lib"
  export LD_LIBRARY_PATH=$python_base
  export PYTHONPATH="$python_base/python2.7/site-packages"

  if ! $opt_fast; then
    git_clone "$REPO_BASE/lldb.git" "$lldb_path" "$UPSTREAM_BRANCH"

    cherry_pick_patches "$lldb_path"

    # Sync lldb libs to local build dir
    rsync -a /usr/lib/x86_64-linux-gnu/       "$python_base"
    rsync -a /usr/lib/llvm-3.9/lib/python2.7/ "$python_base/python2.7"
    rsync -a "$python_base/liblldb-3.9.so"    "$python_base/liblldb.so"

    # Fix broken python lldb symlinks
    cd "$PYTHONPATH/lldb"
    for path in $( ls *.so* ); do
      new_link="$(readlink "$path" | sed 's,x86_64-linux-gnu/,,g' | sed 's,../../../../../,../../../,g')"
      rm "$path"
      ln -s "$new_link" "$path"
    done
  fi
else
  die "Testing is only supported on CentOS and Ubuntu."
fi

if [ -n "${LLDB_EXE-}" ] ; then
  lldb_exe=${LLDB_EXE}
fi

cd "$lldb_path/test"

blacklist_dir="$top/Support/Testing/Blacklists"
args=(-q --executable "$lldb_exe" -u CXXFLAGS -u CFLAGS -C "$cc_exe" -v --exclude "$blacklist_dir/general.blacklist")

if [ -n "${TARGET-}" ]; then
  if [[ "${TARGET}" == "Linux-X86_64" ]]; then
    args+=("--arch=x86_64")
  elif [[ "${TARGET}" == "Linux-X86" || "${TARGET}" == "Android-X86" ]]; then
    args+=("--arch=i386")
  elif [[ "${TARGET}" == "Android-ARM" ]]; then
    args+=("--arch=arm")
  fi
else
  # If this is a developer run (not running on Travis with a $TARGET), run all
  # tests by default.
  LLDB_TESTS="${LLDB_TESTS-all}"

  if [[ "$(uname -m)" = "x86_64" ]]; then
    args+=("--arch=x86_64")
  elif [[ "$(uname -m)" =~ "i[3-6]86" ]]; then
    args+=("--arch=i386")
  fi
fi

if $opt_strace; then
  cat >"$build_dir/ds2-strace.sh" <<EEOOFF
#!/usr/bin/env bash
exec strace -o $build_dir/ds2-strace.log $build_dir/ds2 "\$@"
EEOOFF
  chmod +x "$build_dir/ds2-strace.sh"
  server_path="$build_dir/ds2-strace.sh"
else
  server_path="${DEBUGSERVER_PATH-$build_dir/ds2}"
fi

if [[ "${PLATFORM-}" = "1" ]]; then
  if [[ "$platform_name" = "linux" ]]; then
    working_dir="$build_dir"
  elif [[ "$platform_name" = "android" ]]; then
    working_dir="/data/local/tmp"
    args+=(--exclude "$blacklist_dir/android.blacklist")
  fi

  server_port="12345"

  args+=("--platform-name=remote-$platform_name" "--platform-url=connect://localhost:$server_port"
         "--platform-working-dir=$working_dir" "--no-multiprocess" --exclude "$blacklist_dir/platform.blacklist")

  server_args=("p")

  if [[ "$server_path" = *"lldb-server"* ]] ; then
    server_args+=("--server" "--listen" "$server_port")
  fi

  if $opt_log; then
    server_args+=("--remote-debug" "--log-file=$working_dir/$(basename "$server_path").log")
  fi

  if [[ "$platform_name" = "linux" ]]; then
    "$server_path" "${server_args[@]}" &
    add_exit_handler kill -9 $!
  elif [[ "$platform_name" = "android" ]]; then
    debug_libdir="/tmp/debug-data/"
    mkdir -p "$debug_libdir"
    debug_libs=("/system/bin/linker" "/system/lib/libc.so" "/system/lib/libm.so" "/system/lib/libstdc++.so")
    for lib in "${debug_libs[@]}" ; do
      adb pull "$lib" "$debug_libdir"
    done
    adb push "$server_path" "$working_dir"
    adb shell "$working_dir/ds2" "${server_args[@]}" &
  fi
  sleep 3
else
  if $opt_log; then
    export LLDB_DEBUGSERVER_LOG_FILE="$build_dir/ds2.log"
    export LLDB_DEBUGSERVER_EXTRA_ARG_1="--remote-debug"
  fi

  export LLDB_DEBUGSERVER_PATH="$server_path"
fi

export LLDB_TEST_TIMEOUT=8m

if [ "$LLDB_TESTS" != "all" ]; then
  args+=(-p "$LLDB_TESTS")
fi

python2.7 dotest.py "${args[@]}"
