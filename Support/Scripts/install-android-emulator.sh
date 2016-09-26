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

target_arch="${1-arm}"
api_level="${2-21}"

case "$(uname)" in
  "Linux")  platform_name="linux"; package_extension="tgz";;
  "Darwin") platform_name="macosx"; package_extension="zip";;
  *)        die "This script works only on Linux and macOS.";;
esac

case "${target_arch}" in
  "arm") emulator_image_arch="armeabi-v7a";;
  "x86") emulator_image_arch="x86";;
  *)     die "Unknown architecture '${target_arch}'.";;
esac

android_sdk_version="r23"
android_script="/tmp/android-sdk-${platform_name}/tools/android"

if [ ! -f "$android_script" ]; then
  android_sdk_package="android-sdk_${android_sdk_version}-${platform_name}.${package_extension}"
  wget "https://dl.google.com/android/${android_sdk_package}" -P /tmp
  case "${package_extension}" in
    "zip") unzip -d /tmp "/tmp/${android_sdk_package}";;
    "tgz") tar -C /tmp -xvf "/tmp/${android_sdk_package}";;
    *) die "Unknown archive extension."
  esac
fi

echo "y" | "$android_script" update sdk -u -a --filter "android-${api_level}"

echo "y" | "$android_script" update sdk -u -a --filter "sys-img-${emulator_image_arch}-android-${api_level}"
echo "no" | "$android_script" create avd --force -n test -t "android-${api_level}" --abi "${emulator_image_arch}"
