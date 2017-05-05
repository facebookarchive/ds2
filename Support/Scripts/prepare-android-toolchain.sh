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

ndk_release="r13b"
install_dir="/tmp/aosp-toolchain"

case "$(uname)" in
  "Linux")  host="linux-x86_64";;
  "Darwin") host="darwin-x86_64";;
  *)        die "This script works only on Linux and macOS.";;
esac

while [ $# -ge 1 ] && [[ "$1" =~ --.* ]]; do
  case "$1" in
    --ndk-path)
      shift
      ndk_path="$1"
      ;;
  esac
  shift
done

[ $# -ge 1 ] || die "usage: $0 ARCH[:API_LEVEL]..."

if [ -z "${ndk_path-}" ]; then
  tmpdir="$(mktemp -d)"
  cd "${tmpdir}"

  ndk_package_base="android-ndk-${ndk_release}"
  ndk_package_archive="${ndk_package_base}-${host}.zip"

  wget "https://dl.google.com/android/repository/${ndk_package_archive}"
  unzip -q "${ndk_package_archive}"

  ndk_path="$(realpath "$ndk_package_base")"
fi

for arg in "$@"; do
  if [[ "$arg" == *:* ]]; then
    target_arch="${arg%%:*}"
    api_level="${arg##*:}"
  else
    target_arch="${arg}"
    api_level="21"
  fi

  toolchain_install_dir="${install_dir}/${target_arch}-${api_level}"

  "${ndk_path}/build/tools/make_standalone_toolchain.py" \
      --arch "${target_arch}" \
      --api "${api_level}"    \
      --install-dir "${toolchain_install_dir}" \
      --force

  rm -f "${install_dir}/${target_arch}"
  ln -s "${toolchain_install_dir}" "${install_dir}/${target_arch}"
done
