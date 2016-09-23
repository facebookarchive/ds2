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

source "$(dirname "$0")/common.sh"

[ $# -le 2 ] || die "usage: $0 [ARCH] [PLATFORM]"

case "$(uname)" in
  "Linux")  host="linux-x86";;
  "Darwin") host="darwin-x86";;
  *)        die "This script works only on Linux and macOS.";;
esac

target_arch="${1-arm}"
api_level="${2-21}"

case "${target_arch}" in
  arm)
    toolchain_triple="arm-linux-androideabi"
    ;;

  i[3-6]86|x86)
    # API levels 21 and up have a dual-arch x86 toolchain. Just use that.
    if [ "${api_level}" -ge 21 ]; then
      exec "$0" x86_64 "${api_level}"
    fi
    target_arch="x86"
    toolchain_triple="x86_64-linux-android"
    ;;

  arm64|aarch64)
    [ "${api_level}" -ge 21 ] || die "Minimum supported api level for ${target_arch} is 21."
    target_arch="arm64"
    toolchain_triple="aarch64-linux-android"
    ;;

  amd64|x86_64)
    [ "${api_level}" -ge 21 ] || die "Minimum supported api level for ${target_arch} is 21."
    target_arch="x86_64"
    toolchain_triple="x86_64-linux-android"
    ;;

  *) die "Unknown architecture '${target_arch}'.";;
esac

[ "${api_level}" -ge 16 ] || die "Minimum supported api level is 16."

aosp_platform="android-${api_level}"
toolchain_version="4.9"
toolchain_path="/tmp/aosp-toolchain/${toolchain_triple}-${toolchain_version}"
aosp_ndk_path="/tmp/aosp-toolchain/aosp-ndk"

aosp_prebuilt_clone() {
  git_clone "https://android.googlesource.com/platform/prebuilts/$1" "$2"
}

clobber_dir() {
  rm -rf "$2"
  mkdir -p "$(dirname "$2")"
  cp -R "$(realpath "$1")" "$2"
}

mkdir -p "$(dirname "${toolchain_path}")"

case "${target_arch}" in
  x86_64) aosp_prebuilt_clone "gcc/${host}/x86/${toolchain_triple}-${toolchain_version}" "${toolchain_path}";;
  arm64)  aosp_prebuilt_clone "gcc/${host}/aarch64/${toolchain_triple}-${toolchain_version}" "${toolchain_path}";;
  *)      aosp_prebuilt_clone "gcc/${host}/${target_arch}/${toolchain_triple}-${toolchain_version}" "${toolchain_path}";;
esac
aosp_prebuilt_clone "ndk" "${aosp_ndk_path}"

# Copy sysroot.
clobber_dir "${aosp_ndk_path}/current/platforms/${aosp_platform}/arch-${target_arch}" "${toolchain_path}/sysroot"
if [ "${target_arch}" == "x86_64" ]; then
  # For x86_64, libraries are in `usr/lib64/`; we need to copy 32-bit versions
  # of these libraries to `usr/lib/`.
  clobber_dir "${aosp_ndk_path}/current/platforms/${aosp_platform}/arch-x86/usr/lib" "${toolchain_path}/sysroot/usr/lib"
fi

# gcc version isn't guaranteed to match toolchain version - for example, 4.9 vs 4.9.x
gcc_version="$( "${toolchain_path}/bin/${toolchain_triple}-gcc" --version | awk 'NR==1{print $3}' )"

# Copy C++ stuff
clobber_dir "${aosp_ndk_path}/current/sources/cxx-stl/gnu-libstdc++/${toolchain_version}/include" "${toolchain_path}/include/c++/${gcc_version}"
copy_cpp_binaries() {
  clobber_dir "${aosp_ndk_path}/current/sources/cxx-stl/gnu-libstdc++/${toolchain_version}/libs/$1/libgnustl_static.a" "${toolchain_path}/${toolchain_triple}/$2/libstdc++.a"
  clobber_dir "${aosp_ndk_path}/current/sources/cxx-stl/gnu-libstdc++/${toolchain_version}/libs/$1/libgnustl_shared.so" "${toolchain_path}/${toolchain_triple}/$2/libgnustl_shared.so"
  clobber_dir "${aosp_ndk_path}/current/sources/cxx-stl/gnu-libstdc++/${toolchain_version}/libs/$1/libsupc++.a" "${toolchain_path}/${toolchain_triple}/$2/libsupc++.a"
}
copy_cpp_bits() {
  clobber_dir "${aosp_ndk_path}/current/sources/cxx-stl/gnu-libstdc++/${toolchain_version}/libs/$1/include/bits" "${toolchain_path}/include/c++/${gcc_version}/${toolchain_triple}/$2"
}
case "${target_arch}" in
  arm)
    copy_cpp_binaries "armeabi" "lib"
    copy_cpp_bits "armeabi" "bits"
    ;;
  arm64)
    copy_cpp_binaries "arm64-v8a" "lib"
    copy_cpp_bits "arm64-v8a" "bits"
    ;;
  x86|x86_64)
    copy_cpp_binaries "x86_64" "lib64"; copy_cpp_binaries "x86" "lib"
    copy_cpp_bits "x86_64" "bits";      copy_cpp_bits "x86" "32/bits"
    ;;
esac
