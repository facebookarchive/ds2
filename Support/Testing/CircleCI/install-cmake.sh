#!/usr/bin/env bash
## Copyright (c) Meta Platforms, Inc. and affiliates.
##
## This source code is licensed under the Apache License v2.0 with LLVM
## Exceptions found in the LICENSE file in the root directory of this
## source tree.

if [ ! -e "/tmp/cmake.tar.gz" ] ; then
  wget --continue --output-document="/tmp/cmake.tar.gz" "https://cmake.org/files/v3.12/cmake-3.12.1-Linux-x86_64.tar.gz"
fi

tar --strip-components=1 -xf "/tmp/cmake.tar.gz" -C "/usr/local"
