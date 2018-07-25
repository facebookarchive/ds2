#!/usr/bin/env python3
#
# Copyright (c) 2014-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the University of Illinois/NCSA Open
# Source License found in the LICENSE file in the root directory of this
# source tree. An additional grant of patent rights can be found in the
# PATENTS file in the same directory.
#

from typing import Optional, Callable
import argparse
import os
import shutil
import sys
import urllib.request
import zipfile

has_progress_bar = False

try:
    import progress.bar
except ImportError:
    print("`pip3 install progress` to see progress bars")
else:
    has_progress_bar = True

    class ProgressBar(progress.bar.Bar):
        def urllib_reporthook(
            self, chunk_number: int, maximum_chunks: int, total_size: int
        ):
            self.max = total_size / maximum_chunks
            self.next()

        def unzip_reporthook(self, count: int):
            self.max = count
            self.next()


def unzip(source: str, destination: str, reporthook: Optional[Callable[[int],None]] = None):
    def unzip_file(zf, info, destination):
        dest_path = zf.extract(info.filename, path=destination)

        perm = info.external_attr >> 16
        os.chmod(dest_path, perm)

    with zipfile.ZipFile(source, "r") as f:
        for info in f.infolist():
            if reporthook is not None:
                reporthook(len(f.infolist()))
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
        print("Unsupported platform: {}".format(sys.platform))
        sys.exit(1)

    ndk_version = "r16b"
    ndk_package_base = "android-ndk-{}".format(ndk_version)
    ndk_package_archive = "{}-{}.zip".format(ndk_package_base, host)
    ndk_url = "https://dl.google.com/android/repository/{}".format(ndk_package_archive)
    ndk_download_path = "{}/{}".format(args.download_prefix, ndk_package_archive)
    ndk_tmp_unzip = "/tmp/android-ndk-unzip"
    ndk_unzip_path = "{}/android-ndk".format(args.download_prefix)

    if has_progress_bar:
        bar = ProgressBar("Downloading NDK")
        urllib.request.urlretrieve(ndk_url, ndk_download_path, bar.urllib_reporthook)
        bar.finish()

        bar = ProgressBar("Unzipping NDK")
        unzip(ndk_download_path, ndk_tmp_unzip, bar.unzip_reporthook)
        bar.finish()
    else:
        print("Downloading NDK...")
        urllib.request.urlretrieve(ndk_url, ndk_download_path)
        print("Done.")
        print("Unzipping NDK...")
        unzip(ndk_download_path, ndk_tmp_unzip)
        print("Done.")

    print("Moving ndk to {}".format(ndk_unzip_path))
    shutil.move(ndk_tmp_unzip + "/android-ndk-r16b", ndk_unzip_path)
    print("Done.")


if __name__ == "__main__":
    main()
