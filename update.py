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


def run(args, cwd=None) -> str:
    """run a command with some niceties"""
    cd = "" if cwd is None else f"cd {shlex.quote(str(cwd))} && "
    print(f"+ {cd}{shlex.join(str(a) for a in args)}")
    p = subprocess.run(args, stdout=subprocess.PIPE, cwd=cwd, check=False, text=True)
    sys.stdout.write(p.stdout)
    p.check_returncode()
    return p.stdout


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
                "--depth=1",
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

        # note where this was derived from
        metadata = workdir / "metadata.txt"
        commit = run(["git", "rev-parse", "HEAD"], workdir)
        metadata.write_text(
            f"built from {commit} of https://github.com/smattr/{options.project}",
            encoding="utf-8"
        )
        run(["sudo", "mv", metadata, f"/opt/{options.project}/metadata.txt"])

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
