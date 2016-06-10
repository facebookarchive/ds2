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

dist_packages = []
local_packages = []
pip_packages = []

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

if target == 'Registers':
    dist_packages.append('g++-5')
elif target in linux_packages:
    if "CentOS Linux" in platform.linux_distribution():
        dist_packages.append('gcc')
    else:
        # Install gcc even when using clang, so we can run llgs tests.
        dist_packages.append(linux_packages[target])
elif target == 'MinGW-X86':
    dist_packages.append('g++-mingw-w64-i686')
elif target in android_toolchains:
    # Android builds get the toolchain from AOSP.
    check_call('./Support/Scripts/prepare-android-toolchain.sh "%s"' % android_toolchains[target], shell=True)
elif target in tizen_packages:
    dist_packages.append(tizen_packages[target])

if 'Darwin' in target:
    dist_packages.append('cmake')
    if os.getenv('CLANG') == '0':
        dist_packages.append('gcc')

if target != 'Style':
    dist_packages.append('flex')
    dist_packages.append('bison')

# Running LLDB tests requires an install of lldb (for the tests to be able to
# use the lldb python library without us building it).
if os.getenv('LLDB_TESTS') != None or os.getenv('GDB_TESTS') != None:
    pip_packages.append('six')
    dist_packages.append('python-pip')
    dist_packages.append('swig')
    if "CentOS Linux" in platform.linux_distribution():
        dist_packages.append('libedit-devel')
        dist_packages.append('libxml2-devel')
        dist_packages.append('ncurses-devel')
        dist_packages.append('python-devel')
    else:
        # The order of packages here is important, since they're being installed manually
        local_packages.append('libllvm3.8')
        local_packages.append('llvm-3.8-runtime')
        local_packages.append('llvm-3.8')
        local_packages.append('llvm-3.8-dev')
        if os.getenv('LLDB_TESTS') != None:
            local_packages.append('liblldb-3.8')
            local_packages.append('lldb-3.8')
            local_packages.append('liblldb-3.8-dev')
            local_packages.append('python-lldb-3.8')
        if os.getenv('CLANG') == '1':
            dist_packages.append('libobjc-4.8-dev')
            local_packages.append('libclang1-3.8')
            local_packages.append('libclang-common-3.8-dev')
            local_packages.append('clang-3.8')

if os.getenv('GDB_TESTS') != None:
    dist_packages.append('dejagnu')
    dist_packages.append('gdb')

if os.getenv('COVERAGE') == '1':
    dist_packages.append('python-pip')
    dist_packages.append('python-yaml')
    pip_packages.append('pyyaml')
    pip_packages.append('cpp-coveralls')

if len(dist_packages) > 0:
    if 'Darwin' in target:
        # brew upgrade/install might die if one pkg is already install
        check_call('brew install "%s" || true' % '" "'.join(dist_packages), shell=True)
        check_call('brew upgrade "%s" || true' % '" "'.join(dist_packages), shell=True)
    elif "CentOS Linux" in platform.linux_distribution():
        check_call('sudo yum install -y "%s"' % '" "'.join(dist_packages), shell=True)
    else:
        check_call('sudo apt-get install -y "%s"' % '" "'.join(dist_packages), shell=True)

if len(local_packages) > 0:
    if 'Darwin' in target:
        # brew upgrade/install might die if one pkg is already install
        check_call('brew install "%s" || true' % '" "'.join(local_packages), shell=True)
        check_call('brew upgrade "%s" || true' % '" "'.join(local_packages), shell=True)
    else:
        # These need to be installed individually to properly satisfy dependencies
        for package in local_packages:
            check_call('sudo dpkg -i ./Support/Testing/Travis/Packages/' + package + '.deb', shell=True)

if len(pip_packages) > 0:
    check_call('sudo pip install --upgrade pip', shell=True)
    for package in pip_packages:
        check_call('pip install --user ' + package, shell=True)
