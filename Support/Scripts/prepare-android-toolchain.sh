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

set -eu
die() { echo "error:" "$@" >&2; exit 1; }

[ $# -eq 1 ] || die "usage: $0 [ arm | x86 ]"

case "$(uname)" in
  "Linux")  host="linux-x86";;
  "Darwin") host="darwin-x86";;
  *)        die "This script works only on Linux and Mac OS X.";;
esac

case "$1" in
  "arm")  toolchain_arch="arm"; toolchain_subarch="armeabi"; toolchain_triple="arm-linux-androideabi";;
  "x86")  toolchain_arch="x86"; toolchain_subarch="x86"; toolchain_triple="x86_64-linux-android";;
  *)      die "Unknown architecture '$1'.";;
esac

toolchain_version="4.8"
toolchain_path="/tmp/aosp-toolchain/${toolchain_triple}-${toolchain_version}"
aosp_platform="android-21"

aosp_prebuilt_clone() {
  git clone --depth=1 "https://android.googlesource.com/platform/prebuilts/$1" "$2"
}

[ -d "${toolchain_path}" ] && die "${toolchain_path} already exists."
mkdir -p "$(dirname "${toolchain_path}")"
aosp_prebuilt_clone "gcc/${host}/${toolchain_arch}/${toolchain_triple}-${toolchain_version}" "${toolchain_path}"

aosp_ndk_path="${toolchain_path}/aosp-ndk"
aosp_prebuilt_clone "ndk" "${aosp_ndk_path}"

# Copy sysroot.
cp -R "${aosp_ndk_path}/9/platforms/android-21/arch-${toolchain_arch}" "${toolchain_path}/sysroot"

# Copy C++ stuff
mkdir -p "${toolchain_path}/include/c++"
cp -R "${aosp_ndk_path}/9/sources/cxx-stl/gnu-libstdc++/${toolchain_version}/include" "${toolchain_path}/include/c++/${toolchain_version}"
mkdir -p "${toolchain_path}/include/c++/${toolchain_version}/${toolchain_triple}"
cp -R "${aosp_ndk_path}/9/sources/cxx-stl/gnu-libstdc++/${toolchain_version}/libs/${toolchain_subarch}/libgnustl_static.a" "${toolchain_path}/${toolchain_triple}/lib/libstdc++.a"
cp -R "${aosp_ndk_path}/9/sources/cxx-stl/gnu-libstdc++/${toolchain_version}/libs/${toolchain_subarch}/libgnustl_shared.so" "${toolchain_path}/${toolchain_triple}/lib/libgnustl_shared.so"
cp -R "${aosp_ndk_path}/9/sources/cxx-stl/gnu-libstdc++/${toolchain_version}/libs/${toolchain_subarch}/libsupc++.a" "${toolchain_path}/${toolchain_triple}/lib/libsupc++.a"
cp -R "${aosp_ndk_path}/9/sources/cxx-stl/gnu-libstdc++/${toolchain_version}/libs/${toolchain_subarch}/include/bits" "${toolchain_path}/include/c++/${toolchain_version}/${toolchain_triple}/bits"
