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

host_platform_name=$(get_host_platform_name)
sdk_dir="/tmp/android-sdk-${host_platform_name}"
sdkmanager="${sdk_dir}/tools/bin/sdkmanager"
avdmanager="${sdk_dir}/tools/bin/avdmanager"

if [ ! -f "$sdkmanager" ]; then
  package_name="sdk-tools-${host_platform_name}-4333796.zip"
  wget "https://dl.google.com/android/repository/${package_name}" -P /tmp
  unzip -d "${sdk_dir}" "/tmp/${package_name}"
  rm -f "/tmp/${package_name}"
fi

echo "y" | "${sdkmanager}" --update

case "${target_arch}" in
  "arm")   emulator_image_arch="armeabi-v7a"; api_level="${2-21}";;
  "arm64") emulator_image_arch="arm64-v8a"; api_level="${2-24}";;
  "x86")   emulator_image_arch="x86"; api_level="${2-21}";;
  *)     die "Unknown architecture '${target_arch}'.";;
esac

touch $HOME/.android/repositories.cfg
system_image_package="system-images;android-${api_level};default;${emulator_image_arch}"
echo "y" | "${sdkmanager}" "emulator"
echo "y" | "${sdkmanager}" "platform-tools"
echo "y" | "${sdkmanager}" "platforms;android-${api_level}"
"${sdkmanager}" "${system_image_package}"
echo "no" | "${avdmanager}" create avd \
  --force -n "android-test-${target_arch}" \
  --package "${system_image_package}" \
  --abi "${emulator_image_arch}"
