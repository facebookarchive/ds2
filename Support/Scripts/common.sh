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

set -eu

die() {
    echo "error:" "$@" >&2
    exit 1
}

git_clone() {
    if [ ! -e "$2" ]; then
        git clone --depth 1 "$1" "$2"
    elif [ -d "$2/.git" ]; then
        cd "$2"
        git reset --hard
        git pull --rebase
        cd "$OLDPWD"
    else
        die "'$2' exists and is not a git repository."
    fi
}
