#!/usr/bin/env bash
##
## Copyright (c) 2014-present, Facebook, Inc.
## All right reserved.
##
## This source code is licensed under the University of Illinois/NCSA Open
## Source License found in the LICENSE file in the root directory of this
## source tree. An additional grant of patent rights can be found in the
## PATENTS file in the same directory.

set -eu

# We're running out of disk space on Travis. This is a dirty hack. We shouldn't
# have to do this. In the home directory, perl5 takes up roughly 1.1GB of space
# and otp takes up roughly 3.0GB. They are protected, but we should be able to
# forcibly remove them with sudo.
sudo rm -rf ~/perl5
sudo rm -rf ~/otp

# There are some headers present on Travis macOS bots. These conflict with the
# headers that get installed with gcc so we need to remove them.
if [ "$(uname)" == "Darwin" ]; then
  sudo rm -rf "/usr/local/include/c++"
fi
