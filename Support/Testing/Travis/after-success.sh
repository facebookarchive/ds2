#!/usr/bin/env bash
## Copyright (c) Meta Platforms, Inc. and affiliates.
##
## This source code is licensed under the Apache License v2.0 with LLVM
## Exceptions found in the LICENSE file in the root directory of this
## source tree.

set -eu

if [[ "${TARGET-}" == Darwin-* ]]; then
  exit 0
fi

if [[ "${COVERAGE-}" = "1" ]]; then
  top="$(git rev-parse --show-toplevel)"
  coveralls_args=("-E" ".*/lib/.*" "-E" ".*/include/.*" "-E" ".*/sys/.*" "-E" ".*/lldb/.*"
                  "-E" ".*/deps/.*" "-r" "$top" "-b" "$top/build")
  if [[ "${CLANG-}" = "1" ]]; then
    coveralls_args+=("--gcov" "llvm-cov-5.0" "--gcov-options" "gcov")
  else
    coveralls_args+=("--gcov" "gcov-5")
  fi
  PATH=$PATH:"~/.local/bin" coveralls "${coveralls_args[@]}"
fi
