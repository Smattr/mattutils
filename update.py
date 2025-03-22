#!/usr/bin/env python3

"""
updater for things hosted at https://github.com/smattr
"""

import argparse
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path


def run(args, cwd=None):
    """run a command with some niceties"""
    cd = "" if cwd is None else f"cd {shlex.quote(str(cwd))} && "
    print(f"+ {cd}{shlex.join(str(a) for a in args)}")
    subprocess.check_call(args, cwd=cwd)


def main(args):
    """entry point"""

    # parse command line options
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("project", help="name of software to update")
    parser.add_argument("--version", help="branch or commit to install")
    options = parser.parse_args(args[1:])

    with tempfile.TemporaryDirectory() as t:
        tmp = Path(t)

        workdir = tmp / "working"
        run(
            [
                "git",
                "clone",
                "--recurse-submodules",
                f"https://github.com/smattr/{options.project}",
                workdir,
            ]
        )

        if options.version is not None:
            run(["git", "checkout", options.version], workdir)

        build = tmp / "build"
        run(
            [
                "cmake",
                "-DCMAKE_BUILD_TYPE=Release",
                f"-DCMAKE_INSTALL_PREFIX=/opt/{options.project}",
                "-S",
                workdir,
                "-B",
                build,
            ]
        )
        run(["cmake", "--build", build, "--parallel"])
        run(["sudo", "cmake", "--build", build, "--target", "install"])

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
