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

import os, platform
from subprocess import check_call

packages = []

linux_packages = { 'Linux-ARM':     'g++-4.8-arm-linux-gnueabihf',
                   'Linux-X86':     'g++-5-multilib',
                   'Linux-X86_64':  'g++-5' }

android_toolchains = { 'Android-ARM':       'arm',
                       'Android-ARM64':     'aarch64',
                       'Android-X86':       'x86',
                       'Android-X86_64':    'x86' }

tizen_packages = { 'Tizen-ARM': linux_packages['Linux-ARM'],
                   'Tizen-X86': linux_packages['Linux-X86'] }

target = os.getenv('TARGET')

if target == 'Style':
    packages.append('clang-format-3.8')
elif target == 'Registers':
    packages.append('g++-5')
    packages.append('clang-format-3.8')
elif target in linux_packages:
    if "CentOS Linux" in platform.linux_distribution():
        packages.append('gcc')
    else:
        # Install gcc even when using clang, so we can run llgs tests.
        packages.append(linux_packages[target])
        if os.getenv('CLANG') == '1':
            packages.append('clang-3.7')
elif target == 'MinGW-X86':
    packages.append('g++-mingw-w64-i686')
elif target in android_toolchains:
    # Android builds get the toolchain from AOSP.
    check_call('./Support/Scripts/prepare-android-toolchain.sh "%s"' % android_toolchains[target], shell=True)
elif target in tizen_packages:
    packages.append(tizen_packages[target])

if 'Darwin' in target:
    packages.append('cmake')
    if os.getenv('CLANG') == '0':
        packages.append('gcc')

if target != 'Style':
    packages.append('flex')
    packages.append('bison')

# Running LLDB tests requires an install of lldb (for the tests to be able to
# use the lldb python library without us building it).
if os.getenv('LLDB_TESTS') != None:
    packages.append('swig')
    if 'Darwin' in target:
        packages.append('gcc')
        packages.append('libedit')
        packages.append('ninja')
        packages.append('libxml2')
    if "CentOS Linux" in platform.linux_distribution():
        packages.append('libedit-devel')
        packages.append('libxml2-devel')
        packages.append('ncurses-devel')
        packages.append('python-devel')
    else:
        packages.append('lldb-3.7')
        packages.append('liblldb-3.7')
        packages.append('python-lldb-3.7')

if os.getenv('COVERAGE') == '1':
    packages.append('python-pip')
    packages.append('python-yaml')

if "Ubuntu" in platform.linux_distribution():
    packages.append('realpath')

print('Install: {}'.format(packages))

if len(packages) > 0:
    if 'Darwin' in target:
        # brew upgrade/install might die if one pkg is already install
        check_call('brew install "%s" || true' % '" "'.join(packages), shell=True)
        check_call('brew upgrade "%s" || true' % '" "'.join(packages), shell=True)
    elif "CentOS Linux" in platform.linux_distribution():
        check_call('sudo yum install -y "%s"' % '" "'.join(packages), shell=True)
    else:
        check_call('sudo apt-get install -y "%s"' % '" "'.join(packages), shell=True)

if os.getenv('COVERAGE') == '1':
    check_call('sudo pip install --upgrade pip', shell=True)
    check_call('pip install --user pyyaml', shell=True)
    check_call('pip install --user cpp-coveralls', shell=True)
