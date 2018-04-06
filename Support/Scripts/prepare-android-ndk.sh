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

ndk_release="r16b"

case "$(uname)" in
  "Linux")  host="linux-x86_64";;
  "Darwin") host="darwin-x86_64";;
  *)        die "This script works only on Linux and macOS.";;
esac

[ $# -le 0 ] || die "usage: $0"

cd "/tmp"

ndk_package_base="android-ndk-${ndk_release}"
ndk_package_archive="${ndk_package_base}-${host}.zip"

wget "https://dl.google.com/android/repository/${ndk_package_archive}"
unzip -q "${ndk_package_archive}"
rm -f "${ndk_package_archive}"

rm -rf "$(get_android_ndk_dir)"
mv "${ndk_package_base}" "$(get_android_ndk_dir)"
