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

top="$(git rev-parse --show-toplevel)"
source "$top/Support/Scripts/common.sh"

if [[ "$TARGET" == Android-* && -n "${LLDB_TESTS-}" ]]; then
  case "$(uname)" in
    "Linux")  platform_name="linux";;
    "Darwin") platform_name="darwin";;
  esac

  case "${TARGET}" in
    "Android-ARM") emulator_arch="emulator64-arm";;
    "Android-ARM64") emulator_arch="emulator";;
    "Android-X86") emulator_arch="emulator64-x86";;
    *)             die "Unknown target '${TARGET}'.";;
  esac

  sdk_dir="/tmp/android-sdk-${platform_name}"
  PATH="${sdk_dir}/platform-tools/:$PATH"
  emulator="${sdk_dir}/emulator/${emulator_arch}"
  qt_lib_path="${sdk_dir}/emulator/lib64/qt/lib/"
  LD_LIBRARY_PATH="${qt_lib_path}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" "$emulator" -avd test -gpu off -no-window &
  echo "WHICH ADB?"
  which adb
  echo "WAIT FOR DEVICE"
  adb wait-for-device
  echo "DEVICES?"
  adb devices
fi
