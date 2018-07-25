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
pip_packages = []

linux_packages = { 'Linux-ARM':     'g++-4.8-arm-linux-gnueabihf',
                   'Linux-X86':     'g++-5-multilib',
                   'Linux-X86_64':  'g++-5' }

android_toolchains = { 'Android-ARM':       'arm',
                       'Android-ARM64':     'arm64',
                       'Android-X86':       'x86',
                       'Android-X86_64':    'x86_64' }

tizen_packages = { 'Tizen-ARM': linux_packages['Linux-ARM'],
                   'Tizen-X86': linux_packages['Linux-X86'] }

host = os.uname()[0]
target = os.getenv('TARGET')

dist_packages.append('flex')
dist_packages.append('bison')

if host == 'Darwin':
    dist_packages.append('ninja')
    dist_packages.append('cmake')
    if os.getenv('CLANG') != '1':
        dist_packages.append('gcc')
elif host == 'Linux':
    if os.getenv("LINUX_DISTRO") == "trusty":
        dist_packages.append('ninja-build')
    else:
        dist_packages.append('ninja')

    if target == 'Style' or target == 'Registers':
        dist_packages.append('clang-format-5.0')
    elif target == 'Documentation':
        dist_packages.append('doxygen')
        dist_packages.append('graphviz')
    elif target in linux_packages:
        if "CentOS Linux" in platform.linux_distribution():
            dist_packages.append('gcc')
        else:
            # Install gcc even when using clang, so we can run lldb tests.
            dist_packages.append(linux_packages[target])
    elif target == 'MinGW-X86':
        dist_packages.append('g++-mingw-w64-i686')
    elif target in tizen_packages:
        dist_packages.append(tizen_packages[target])

if target in android_toolchains:
    # Android builds get the toolchain from AOSP and use platform 21 by default.
    check_call('./Support/Scripts/prepare-android-ndk.py', shell=True)
    if os.getenv('LLDB_TESTS') != None:
        dist_packages.append('default-jdk')

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
        if os.getenv('LLDB_TESTS') != None:
            dist_packages.append('lldb-5.0')
            dist_packages.append('python-lldb-5.0')
        if os.getenv('CLANG') == '1':
            dist_packages.append('clang-5.0')

if os.getenv('GDB_TESTS') != None:
    dist_packages.append('dejagnu')
    dist_packages.append('gdb')

if os.getenv('COVERAGE') == '1':
    dist_packages.append('python-pip')
    dist_packages.append('python-yaml')
    dist_packages.append('ggcov')
    pip_packages.append('pyyaml')
    pip_packages.append('cpp-coveralls')

if len(dist_packages) > 0:
    if host == 'Darwin':
        check_call('brew reinstall "%s"' % '" "'.join(dist_packages), shell=True)
    else:
        if "CentOS Linux" in platform.linux_distribution():
            check_call('sudo yum install -y "%s"' % '" "'.join(dist_packages), shell=True)
        else:
            check_call('sudo apt-get install -y "%s"' % '" "'.join(dist_packages), shell=True)
            check_call('sudo apt-get clean', shell=True)

if len(pip_packages) > 0:
    check_call('sudo pip install --no-use-wheel --upgrade pip', shell=True)
    for package in pip_packages:
        check_call('pip install --no-use-wheel --upgrade --user ' + package, shell=True)

# This step must happen after package installation, as it requires java be installed.
if target in android_toolchains and os.getenv('LLDB_TESTS') != None:
    check_call('./Support/Scripts/install-android-emulator.sh %s' % android_toolchains[target], shell=True)
