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

build_type = os.getenv('BUILD_TYPE')
target = os.getenv('TARGET')

packages = ['cmake']

if build_type == 'Style':
    packages.append('clang-format-3.6')

linux_packages = { 'Linux-ARM':     'g++-arm-linux-gnueabi',
                   'Linux-X86':     'g++-4.8-multilib',
                   'Linux-X86_64':  'g++-4.8' }
if target in linux_packages:
    packages.append(linux_packages[target])

# Running LLGS tests requires an install of lldb (for the tests to be able to
# use the lldb python library without us building it).
if os.getenv('LLGS_TESTS') == '1':
    packages.append('swig')
    packages.append('lldb-3.3')

if os.getenv('REGSGEN') == '1':
    packages.append('flex')
    packages.append('bison')

check_call('sudo apt-get install -y "%s"' % '" "'.join(packages), shell=True)

android_toolchains = { 'Android-ARM':       'arm',
                       'Android-ARM64':     'aarch64',
                       'Android-X86':       'x86',
                       'Android-X86_64':    'x86' }
if target in android_toolchains:
    check_call('./Support/Scripts/prepare-android-toolchain.sh "%s"' % android_toolchains[target], shell=True)
