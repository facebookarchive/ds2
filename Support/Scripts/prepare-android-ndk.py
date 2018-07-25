#!/usr/bin/env python3
##
## Copyright (c) 2014-present, Facebook, Inc.
## All rights reserved.
##
## This source code is licensed under the University of Illinois/NCSA Open
## Source License found in the LICENSE file in the root directory of this
## source tree. An additional grant of patent rights can be found in the
## PATENTS file in the same directory.
##

import sys
import argparse
from typing import Any
import urllib.request
import zipfile
import os
import shutil


def unzip(source, destination):
    def unzip_file(zf, info, destination):
        dest_path = zf.extract(info.filename, path=destination)

        perm = info.external_attr >> 16
        os.chmod(dest_path, perm)

    with zipfile.ZipFile(source, "r") as f:
        for info in f.infolist():
            unzip_file(f, info, destination)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--download-prefix",
        help="the directory prefix for the ndk installation",
        default="/tmp",
    )

    args = parser.parse_args()
    return args


def main():
    args = parse_arguments()

    if sys.platform == "linux":
        host = "linux-x86_64"
    elif sys.platform == "darwin":
        host = "darwin-x86_64"
    else:
        print("Uninmplemented platform: {}".format(sys.platform))
        sys.exit(1)

    ndk_version = "r16b"
    ndk_package_base = "android-ndk-{}".format(ndk_version)
    ndk_package_archive = "{}-{}.zip".format(ndk_package_base, host)
    ndk_url = "https://dl.google.com/android/repository/{}".format(ndk_package_archive)
    ndk_download_path = "{}/{}".format(args.download_prefix, ndk_package_archive)
    ndk_tmp_unzip = "/tmp/android-ndk-unzip"
    ndk_unzip_path = "{}/android-ndk".format(args.download_prefix)

    urllib.request.urlretrieve(ndk_url, ndk_download_path)

    unzip(ndk_download_path, ndk_tmp_unzip)

    shutil.move(ndk_tmp_unzip + "/android-ndk-r16b", ndk_unzip_path)


if __name__ == "__main__":
    main()
