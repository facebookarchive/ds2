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

import os
import argparse
import subprocess
from typing import Dict, Any, Tuple
import sys
import copy


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--target", help="the target emulator to run")
    parser.add_argument(
        "--download-path",
        help="the directory where the ndk and sdk are installed",
        default="/tmp",
    )

    args = parser.parse_args()
    return args


def clean_arguments(args: argparse.Namespace) -> argparse.Namespace:
    if args.target:
        os.environ["CIRCLE_JOB"] = args.target
    elif os.environ.get("CIRCLE_JOB", None) is not None:
        args.target = os.environ.get("CIRCLE_JOB", None)
    else:
        print("either the CIRCLE_JOB env var or the --target flag is required")
        sys.exit(1)

    return args


def get_arch_from_target(target: str) -> Tuple[str, str]:
    target_arch_pairs = {
        "Android-ARM": ("emulator64-arm", "arm"),
        "Android-ARM64": ("emulator", "arm64"),
        "Android-x86": ("emulator64-x86", "x86"),
    }
    archs = target_arch_pairs.get(target, None)
    if archs is None:
        print("either the TARGET env var or the --target flag is required")
        sys.exit(1)
    else:
        return archs


def main() -> None:
    args = parse_arguments()
    args = clean_arguments(args)

    emulator_arch, arch = get_arch_from_target(args.target)
    sdk_dir = "{}/android-sdk-{}".format(args.download_path, sys.platform)
    emulator = "{}/emulator/{}".format(sdk_dir, emulator_arch)
    qt_lib_path = "{}/emulator/lib64/qt/lib".format(sdk_dir)

    new_environment = os.environ.copy()
    new_environment["LD_LIBRARY_PATH"] = qt_lib_path

    subprocess.run(
        [
            emulator,
            "-avd",
            "android-test-{}".format(arch),
            "-gpu",
            "off",
            "-no-window",
            "-no-accel",
            "-no-audio",
        ],
        env=new_environment,
    )


if __name__ == "__main__":
    main()
