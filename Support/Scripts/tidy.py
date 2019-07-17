#!/usr/bin/env python3.7

import argparse
import functools
import json
import multiprocessing
import os
import subprocess
import sys
from typing import Optional

# Some files should not be ran through clang-tidy for various reasons e.g. they
# are generated with flex/bison and thus aren't guaranteed to be tidy. Pass
# the `--ignore arg` flag where arg is a substring present in the entire file
# name within the ccdb. e.g. to ignore
# `/Users/lanza/Projects/ds2/mbuild/JSObjects/tokenizer.c` you ccan use
# `--ignore tokenizer`.


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--ignore",
        action="append",
        default=[],
        help="files listed in the ccdb that clang-tidy should ignore",
    )
    args = parser.parse_args()
    return args


def run_clang_tidy(ignores: [str], file: str) -> Optional[subprocess.CompletedProcess]:
    for ignore in ignores:
        if ignore in file:
            return None
    return subprocess.run("clang-tidy -warnings-as-errors='*' " + file, shell=True)


def get_parent_ccdb_from(path: str) -> Optional[str]:
    current_directory = path
    while current_directory != "/":
        if os.path.exists(os.path.join(current_directory, "compile_commands.json")):
            return os.path.join(current_directory, "compile_commands.json")
        else:
            current_directory = os.path.dirname(current_directory)
    return None


def die():
    print(
        """
clang-tidy requires that a compile_commands.json is located in a parent
directory of the file to be checked. If you build in `build` then you should
run `ln -s build/compile_commands.json .`"""
    )
    sys.exit(1)


def main() -> int:
    args = parse_args()

    ccdb_path = get_parent_ccdb_from(os.path.abspath(os.curdir))
    if ccdb_path is None:
        die()

    with open(ccdb_path) as ccdb:
        compile_commands = json.load(ccdb)

        # test to see if there is a ccdb in a parent directory of the first
        # source file since all files should have a common ccdb in a common
        # ancestor
        if get_parent_ccdb_from(compile_commands[0]["file"]) is None:
            die()

        files_to_parse = map(lambda x: x["file"], compile_commands)

        run_clang_tidy_with_ignores = functools.partial(run_clang_tidy, args.ignore)

        cpus = multiprocessing.cpu_count()
        with multiprocessing.Pool(cpus) as p:
            results = p.map(run_clang_tidy_with_ignores, files_to_parse)
            filtered_results = filter(lambda x: x is not None, results)
            return_codes = map(lambda x: x.returncode, filtered_results)
            return any(return_codes)


if __name__ == "__main__":
    sys.exit(main())
