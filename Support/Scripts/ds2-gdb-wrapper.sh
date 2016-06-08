#!/usr/bin/env bash
## Copyright (c) 2014-present, Facebook, Inc.
## All rights reserved.
##
## This source code is licensed under the University of Illinois/NCSA Open
## Source License found in the LICENSE file in the root directory of this
## source tree. An additional grant of patent rights can be found in the
## PATENTS file in the same directory.
##

# This script converts gdbserver arguments into a format ds2 can recognize

ds2_args=("g" "-g") # gdbserver and gdb-compatibility mode
"$(dirname "$0")/../../build/ds2" "${ds2_args[@]}" "$@"
