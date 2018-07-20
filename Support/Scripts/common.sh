#!/usr/bin/env bash
##
## Copyright (c) 2014-present, Facebook, Inc.
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
  case "$(uname)" in
    Darwin) num_cpus=$(sysctl -n hw.ncpu);;
    Linux) num_cpus=$(grep -c "^processor" /proc/cpuinfo);;
    *) die "Unknown OS '$(uname)'.";;
  esac
  echo $(($num_cpus * 5 / 4))
}

realpath() { python -c "import os,sys; print os.path.realpath(sys.argv[1])" "$1"; }

same_dir() {
  [ "$(realpath "$1")" = "$(realpath "$2")" ]
}

exit_handlers="exit"
exit_handler() { set +e; eval "$exit_handlers"; }
trap exit_handler EXIT
add_exit_handler() { exit_handlers="$*; $exit_handlers"; }

linux_distribution() {
  if [ -r /etc/issue ] && grep -q "Ubuntu" "/etc/issue"; then
    echo "ubuntu"
  elif [ -s /etc/centos-release ]; then
    echo "centos"
  else
    echo "unknown"
  fi
}

get_host_platform_name() {
  case "$(uname)" in
    "Linux")  echo "linux";;
    "Darwin") echo "darwin";;
    *)        die "This script only works on Linux and macOS.";;
  esac
}

get_android_ndk_dir() {
  echo "/tmp/android-ndk"
}

check_program_exists() {
  which "$1" >/dev/null 2>&1
}

get_program_path() {
  local program=$(which $1) || die "$1 not found"
  echo "${program}"
}
