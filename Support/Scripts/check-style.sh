#!/usr/bin/env bash
##
## Copyright (c) 2014, Facebook, Inc.
## All rights reserved.
##
## This source code is licensed under the University of Illinois/NCSA Open
## Source License found in the LICENSE file in the root directory of this
## source tree. An additional grant of patent rights can be found in the
## PATENTS file in the same directory.
##

if [ -z "${CLANG_FORMAT-}" ]; then
  CLANG_FORMAT="clang-format"
fi

source "$(dirname "$0")/common.sh"

if [ $# -eq 0 ]; then
  die "usage: $0 PATH..."
fi

dirty=()

for d in "$@"; do
  if [ "$(git status -s "$d" | wc -l)" -ne 0 ]; then
    die "Run this script on a clean repository."
  fi

  find "$d" -type f -exec "$CLANG_FORMAT" -i -style=LLVM {} \;

  dirty+=($(git status -s "$d" | awk '{ print $2 }'))
done

if [ "${#dirty[@]}" -eq 0 ]; then
  echo "No style errors."
  exit 0
else
  for f in "${dirty[@]}"; do
    echo "Style mismatch: $f"
  done
  exit 1
fi
