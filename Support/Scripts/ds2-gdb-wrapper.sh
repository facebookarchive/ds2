#!/usr/bin/env bash
## Copyright (c) Meta Platforms, Inc. and affiliates.
##
## This source code is licensed under the Apache License v2.0 with LLVM
## Exceptions found in the LICENSE file in the root directory of this
## source tree.

# This script converts gdbserver arguments into a format ds2 can recognize

ds2_args=("g" "-g") # gdbserver and gdb-compatibility mode
"$(dirname "$0")/../../build/ds2" "${ds2_args[@]}" "$@"
