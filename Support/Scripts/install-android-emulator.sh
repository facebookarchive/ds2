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

[ $# -eq 0 ] || die "usage: $0"

case "$(uname)" in
  "Linux")  platform_name="linux"; package_extension="tgz";;
  "Darwin") platform_name="macosx"; package_extension="zip";;
  *)        die "This script works only on Linux and macOS.";;
esac

android_sdk_version="r24.4.1"
android_script="/tmp/android-sdk-${platform_name}/tools/android"

if [ ! -f "$android_script" ] ; then
  android_sdk_package="android-sdk_${android_sdk_version}-${platform_name}.${package_extension}"
  wget "https://dl.google.com/android/${android_sdk_package}" -P /tmp
  case "${package_extension}" in
    "zip") unzip -d /tmp "/tmp/${android_sdk_package}";;
    "tgz") tar -C /tmp -xvf "/tmp/${android_sdk_package}";;
    *) die "Unknown archive extension."
  esac

  echo "y" | "$android_script" update sdk -u -a --filter android-21
  echo "y" | "$android_script" update sdk -u -a --filter sys-img-armeabi-v7a-android-21

  echo "no" | "$android_script" create avd --force -n test -t android-21 --abi armeabi-v7a
fi
