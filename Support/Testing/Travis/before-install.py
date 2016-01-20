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

repositories = []
keys = []

target = os.getenv('TARGET')

if target in [ 'Style', 'Registers' ] or os.getenv('CLANG') == '1' or os.getenv('LLGS_TESTS') == '1':
    repositories.append('ppa:ubuntu-toolchain-r/test')
    keys.append('http://llvm.org/apt/llvm-snapshot.gpg.key')
    repositories.append('deb http://llvm.org/apt/trusty/ llvm-toolchain-trusty-3.7 main')
elif target in [ 'Linux-X86', 'Linux-X86_64' ]:
    repositories.append('ppa:ubuntu-toolchain-r/test')

for r in repositories:
  check_call('sudo add-apt-repository -y "%s"' % r, shell=True)

for k in keys:
  check_call('wget -O - "%s" | sudo apt-key add -' % k, shell=True)

check_call('sudo apt-get update', shell=True)
