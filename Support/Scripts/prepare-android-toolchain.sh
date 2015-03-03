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

source "$(dirname "$0")/common.sh"

[ $# -eq 1 ] || die "usage: $0 [ arm | x86 | x86_64 ]"

case "$(uname)" in
  "Linux")  host="linux-x86";;
  "Darwin") host="darwin-x86";;
  *)        die "This script works only on Linux and Mac OS X.";;
esac

case "$1" in
  "arm")
      toolchain_arch="arm" toolchain_subarch="armeabi" toolchain_triple="arm-linux-androideabi"
      ;;
  "x86")
      toolchain_arch="x86"; toolchain_subarch="x86"; toolchain_triple="i686-linux-android"
      gcc_triple="x86_64-linux-android"
      ;;
  "x86_64")
      toolchain_arch="x86_64"; toolchain_subarch="x86_64"; toolchain_triple="x86_64-linux-android"
      gcc_arch="x86"; libdir="lib64"
      ;;
  *)
      die "Unknown architecture '$1'."
      ;;
esac

gcc_arch="${gcc_arch:-${toolchain_arch}}"
gcc_triple="${gcc_triple:-${toolchain_triple}}"
libdir="${libdir:-lib}"

aosp_platform="android-21"
toolchain_version="4.8"
toolchain_path="/tmp/aosp-toolchain/${toolchain_triple}-${toolchain_version}"
aosp_ndk_path="/tmp/aosp-toolchain/aosp-ndk"

aosp_prebuilt_clone() {
  git_clone "https://android.googlesource.com/platform/prebuilts/$1" "$2"
}

clobber_dir() {
  if [ -e "$2" ]; then
    rm -rf "$2"
  fi

  mkdir -p "$(dirname "$2")"
  cp -R "$1" "$2"
}

mkdir -p "$(dirname "${toolchain_path}")"

aosp_prebuilt_clone "gcc/${host}/${gcc_arch}/${gcc_triple}-${toolchain_version}" "${toolchain_path}"
aosp_prebuilt_clone "ndk" "${aosp_ndk_path}"

# Copy sysroot.
clobber_dir "${aosp_ndk_path}/9/platforms/android-21/arch-${toolchain_arch}" "${toolchain_path}/sysroot"

# Copy C++ stuff
clobber_dir "${aosp_ndk_path}/9/sources/cxx-stl/gnu-libstdc++/${toolchain_version}/include" "${toolchain_path}/include/c++/${toolchain_version}"
clobber_dir "${aosp_ndk_path}/9/sources/cxx-stl/gnu-libstdc++/${toolchain_version}/libs/${toolchain_subarch}/libgnustl_static.a" "${toolchain_path}/${toolchain_triple}/${libdir}/libstdc++.a"
clobber_dir "${aosp_ndk_path}/9/sources/cxx-stl/gnu-libstdc++/${toolchain_version}/libs/${toolchain_subarch}/libgnustl_shared.so" "${toolchain_path}/${toolchain_triple}/${libdir}/libgnustl_shared.so"
clobber_dir "${aosp_ndk_path}/9/sources/cxx-stl/gnu-libstdc++/${toolchain_version}/libs/${toolchain_subarch}/libsupc++.a" "${toolchain_path}/${toolchain_triple}/${libdir}/libsupc++.a"
clobber_dir "${aosp_ndk_path}/9/sources/cxx-stl/gnu-libstdc++/${toolchain_version}/libs/${toolchain_subarch}/include/bits" "${toolchain_path}/include/c++/${toolchain_version}/${toolchain_triple}/bits"
