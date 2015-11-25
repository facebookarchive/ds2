#!/usr/bin/env python
##
## Copyright (c) 2014, Facebook, Inc.
## All rights reserved.
##
## This source code is licensed under the University of Illinois/NCSA Open
## Source License found in the LICENSE file in the root directory of this
## source tree. An additional grant of patent rights can be found in the
## PATENTS file in the same directory.
##

import os
from subprocess import check_call

android_toolchains = { 'Android-ARM':       'arm',
                       'Android-ARM64':     'aarch64',
                       'Android-X86':       'x86',
                       'Android-X86_64':    'x86' }

target = os.getenv('TARGET')

# Android builds get the toolchain from AOSP.
if target in android_toolchains:
    check_call('./Support/Scripts/prepare-android-toolchain.sh "%s"' % android_toolchains[target], shell=True)
