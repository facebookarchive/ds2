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

cmake_version="3.7"
cmake_package_name="cmake-${cmake_version}.0-Linux-x86_64"
cmake_archive="${cmake_package_name}.tar.gz"
if [ ! -e "${cmake_archive}" ] ; then
  wget --continue --output-document="/tmp/${cmake_archive}" "https://cmake.org/files/v${cmake_version}/${cmake_archive}"
fi

tar --strip-components=1 -xf "/tmp/${cmake_archive}" -C "/usr/local"
