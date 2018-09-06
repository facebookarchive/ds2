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

if [ ! -e "/tmp/cmake.tar.gz" ] ; then
  wget --continue --output-document="/tmp/cmake.tar.gz" "https://cmake.org/files/v3.12/cmake-3.12.1-Linux-x86_64.tar.gz"
fi

tar --strip-components=1 -xf "/tmp/cmake.tar.gz" -C "/usr/local"
