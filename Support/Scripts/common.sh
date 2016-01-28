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
    src="$1"
    dst="$2"
    branch="${3-}"

    if [ ! -e "$dst" ]; then
        clone_command=(git clone --depth 1)
        if [ -n "$branch" ]; then
            clone_command+=(--branch "$branch")
        fi
        "${clone_command[@]}" "$src" "$dst"
    elif [ -d "$dst/.git" ]; then
        cd "$dst"
        git reset --hard
        if [ -n "$branch" ]; then
            git checkout "$branch"
        fi
        git pull
        cd "$OLDPWD"
    else
        die "'$dst' exists and is not a git repository."
    fi
}

num_cpus() {
  num_cpus=$(grep -c "^processor" /proc/cpuinfo)
  echo $(($num_cpus * 5 / 4))
}

same_dir() {
  [ "$(realpath "$1")" = "$(realpath "$2")" ]
}
