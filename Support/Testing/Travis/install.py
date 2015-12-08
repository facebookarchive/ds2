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

packages = []

linux_packages = { 'Linux-ARM':     'g++-4.7-arm-linux-gnueabi',
                   'Linux-X86':     'g++-4.8-multilib',
                   'Linux-X86_64':  'g++-4.8' }

android_toolchains = { 'Android-ARM':       'arm',
                       'Android-ARM64':     'aarch64',
                       'Android-X86':       'x86',
                       'Android-X86_64':    'x86' }

tizen_toolchains = { 'Tizen-ARM':   'arm',
                     'Tizen-X86':   'x86' }

target = os.getenv('TARGET')

if target == 'Style':
    packages.append('clang-format-3.7')
elif target == 'Registers':
    packages.append('flex')
    packages.append('bison')
    packages.append('g++-4.8')
    packages.append('clang-format-3.7')
elif target in linux_packages:
    # Install gcc even when using clang, so we can run llgs tests.
    packages.append(linux_packages[target])
    if os.getenv('CLANG') == '1':
        packages.append('clang-3.7')
elif target in android_toolchains:
    # Android builds get the toolchain from AOSP.
    check_call('./Support/Scripts/prepare-android-toolchain.sh "%s"' % android_toolchains[target], shell=True)
elif target in tizen_toolchains:
    # Tizen builds use the android toolchain and link statically.
    check_call('./Support/Scripts/prepare-android-toolchain.sh "%s"' % tizen_toolchains[target], shell=True)

# Running LLGS tests requires an install of lldb (for the tests to be able to
# use the lldb python library without us building it).
if os.getenv('LLGS_TESTS') == '1':
    packages.append('swig')
    packages.append('lldb-3.3')

if len(packages) > 0:
    check_call('sudo apt-get install -y "%s"' % '" "'.join(packages), shell=True)
