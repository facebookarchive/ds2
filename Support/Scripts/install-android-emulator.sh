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

set -eu

android_script="/tmp/android-sdk-linux/tools/android"

if [ ! -f "$android_script" ] ; then
  android_sdk_name="android-sdk_r24.4.1-linux"
  wget "https://dl.google.com/android/$android_sdk_name.tgz" -P "/tmp/"
  tar -zxf "/tmp/$android_sdk_name.tgz" -C "/tmp/"

  echo "y" | $android_script update sdk -u -a --filter android-21
  echo "y" | $android_script update sdk -u -a --filter sys-img-armeabi-v7a-android-21

  echo "no" | $android_script create avd --force -n test -t android-21 --abi armeabi-v7a
fi
