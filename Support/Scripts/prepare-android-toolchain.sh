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

[ $# -eq 1 ] || die "usage: $0 [ arm | x86 ]"

case "$(uname)" in
  "Linux")  host="linux-x86";;
  "Darwin") host="darwin-x86";;
  *)        die "This script works only on Linux and Mac OS X.";;
esac

case "$1" in
  "arm")  toolchain_arch="arm";     toolchain_triple="arm-linux-androideabi";;
  "x86")  toolchain_arch="x86_64";  toolchain_triple="x86_64-linux-android";;
  *)      die "Unknown architecture '$1'." ;;
esac

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

aosp_prebuilt_clone "gcc/${host}/$1/${toolchain_triple}-${toolchain_version}" "${toolchain_path}"
aosp_prebuilt_clone "ndk" "${aosp_ndk_path}"

# Copy sysroot. For x86_64, libraries are in `usr/lib64/`; we need to copy 32-bit versions of these libraries to `usr/lib/`.
clobber_dir "${aosp_ndk_path}/9/platforms/${aosp_platform}/arch-${toolchain_arch}" "${toolchain_path}/sysroot"
if [ "${toolchain_arch}" = "x86_64" ]; then
  clobber_dir "${aosp_ndk_path}/9/platforms/${aosp_platform}/arch-x86/usr/lib" "${toolchain_path}/sysroot/usr/lib"
fi

# Copy C++ stuff
clobber_dir "${aosp_ndk_path}/9/sources/cxx-stl/gnu-libstdc++/${toolchain_version}/include" "${toolchain_path}/include/c++/${toolchain_version}"
copy_cpp_binaries() {
  clobber_dir "${aosp_ndk_path}/9/sources/cxx-stl/gnu-libstdc++/${toolchain_version}/libs/$1/libgnustl_static.a" "${toolchain_path}/${toolchain_triple}/$2/libstdc++.a"
  clobber_dir "${aosp_ndk_path}/9/sources/cxx-stl/gnu-libstdc++/${toolchain_version}/libs/$1/libgnustl_shared.so" "${toolchain_path}/${toolchain_triple}/$2/libgnustl_shared.so"
  clobber_dir "${aosp_ndk_path}/9/sources/cxx-stl/gnu-libstdc++/${toolchain_version}/libs/$1/libsupc++.a" "${toolchain_path}/${toolchain_triple}/$2/libsupc++.a"
}
copy_cpp_bits() {
  clobber_dir "${aosp_ndk_path}/9/sources/cxx-stl/gnu-libstdc++/${toolchain_version}/libs/$1/include/bits" "${toolchain_path}/include/c++/${toolchain_version}/${toolchain_triple}/$2"
}
case "$1" in
  "arm")
    copy_cpp_binaries "armeabi" "lib"
    copy_cpp_bits "armeabi" "bits"
    ;;
  "x86")
    copy_cpp_binaries "x86_64" "lib64"; copy_cpp_binaries "x86" "lib"
    copy_cpp_bits "x86_64" "bits";      copy_cpp_bits "x86" "32/bits"
    ;;
esac
