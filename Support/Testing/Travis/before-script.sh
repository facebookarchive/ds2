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
  host_platform_name=$(get_host_platform_name)

  case "${TARGET}" in
    "Android-ARM") emulator_arch="emulator64-arm"; arch="arm";;
    "Android-ARM64") emulator_arch="emulator"; arch="arm64";;
    "Android-X86") emulator_arch="emulator64-x86"; arch="x86";;
    *)             die "Unknown target '${TARGET}'.";;
  esac

  sdk_dir="/tmp/android-sdk-${host_platform_name}"
  emulator="${sdk_dir}/emulator/${emulator_arch}"
  adb="${sdk_dir}/platform-tools/adb"
  qt_lib_path="${sdk_dir}/emulator/lib64/qt/lib/"
  LD_LIBRARY_PATH="${qt_lib_path}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" "$emulator" -avd "android-test-${arch}" -gpu off -no-window -no-accel &
  "$adb" wait-for-device
fi
