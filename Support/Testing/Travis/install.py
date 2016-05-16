#!/usr/bin/env python
##
## Copyright (c) 2014-present, Facebook, Inc.
## All rights reserved.
##
## This source code is licensed under the University of Illinois/NCSA Open
## Source License found in the LICENSE file in the root directory of this
## source tree. An additional grant of patent rights can be found in the
## PATENTS file in the same directory.
##

import os
import platform
import sys
from subprocess import check_call

packages = []

if os.getenv('ANDROID_BUILDS') == '1':
    check_call('./Support/Scripts/prepare-android-toolchain.sh', shell=True)
    sys.exit(0)

if os.getenv('TRAVIS_OS_NAME') == 'linux':
    target_os = 'Linux'
elif os.getenv('TRAVIS_OS_NAME') == 'osx':
    target_os = 'Darwin'
target_arch = os.getenv('ARCH')
target = target_os + '-' + target_arch
compiler = os.getenv('CC')

if target == 'Style':
    packages.append('clang-format-3.8')
elif target == 'Registers':
    packages.append('g++-5')
    packages.append('clang-format-3.8')

elif target_os == 'Linux':
    if compiler == 'clang':
        packages.append('clang-3.7')

    if target_arch == 'X86':
        packages.append('g++-5-multilib')
    elif target_arch == 'X86_64':
        packages.append('g++-5')

    # Required for running the lldb test suite.
    packages.append('swig')
    packages.append('lldb-3.7')
    packages.append('liblldb-3.7')
    packages.append('python-lldb-3.7')

    # Required for sending coverage to coveralls.
    packages.append('python-pip')
    packages.append('python-yaml')
elif target_os == 'Darwin':
    packages.append('cmake')
    if compiler == 'gcc':
        packages.append('gcc')

if target != 'Style':
    packages.append('flex')
    packages.append('bison')

if len(packages) > 0:
    if target_os == 'Linux':
        check_call('sudo apt-get install -y "%s"' % '" "'.join(packages), shell=True)
        # Required for sending coverage to coveralls
        check_call('sudo pip install --upgrade pip', shell=True)
        check_call('pip install --user pyyaml', shell=True)
        check_call('pip install --user cpp-coveralls', shell=True)
    elif target_os == 'Darwin':
        # brew upgrade/install might die if one pkg is already install
        check_call('brew install "%s" || true' % '" "'.join(packages), shell=True)
        check_call('brew upgrade "%s" || true' % '" "'.join(packages), shell=True)

