#!/usr/bin/env python3

"""
test libute with a variety of compiler configurations
"""

import itertools
import platform
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import List


def run(*args: str, cwd: Path):
    print(f"+ cd {cwd} && {' '.join(shlex.quote(str(x)) for x in args)}")
    subprocess.check_call(args, cwd=cwd)


def main(args: List[str]) -> int:

    compiler = []
    if shutil.which("gcc") is not None:
        compiler += ["gcc"]
    if shutil.which("clang") is not None:
        compiler += ["clang"]
    if len(compiler) == 0:
        raise RuntimeError("no compilers found")

    build = ["Debug", None, "RelWithDebInfo"]

    sanitizer = ["address,undefined", "thread,undefined", None]

    for cc, bt, san in itertools.product(compiler, build, sanitizer):
        if cc == "gcc" and san == "thread,undefined":
            # GCC seems not to support TSan?
            continue
        with tempfile.TemporaryDirectory() as t:
            tmp = Path(t)

            # configure the build
            args = ["env", f"CC={cc}"]
            if san is None:
                args += ["CFLAGS=-Werror"]
            else:
                args += [
                    f"CFLAGS=-Werror -fsanitize={san} -fno-sanitize-recover={san} -g"
                ]
            args += ["cmake", "-B", ".", "-S", Path(__file__).parent]
            if bt is not None:
                args += [f"-DCMAKE_BUILD_TYPE={bt}"]
            run(*args, cwd=tmp)

            # compile
            run("cmake", "--build", ".", "--parallel", "--verbose", cwd=tmp)

            # test
            run("cmake", "--build", ".", "--target", "check", cwd=tmp)

            # check volatile loads and stores really did lower to atomic MOVDQA/MOVAPS
            if platform.machine() not in ("amd64", "x86_64"):
                # skip non-SSE environments
                continue
            if bt == "RelWithDebInfo":
                # skip LTO builds that cannot easily be disassembled
                continue
            if san == "thread,undefined":
                # the TSan builds do not use SSE
                continue
            for src in (
                "int128_atomic_load",
                "int128_atomic_store",
                "uint128_atomic_load",
                "uint128_atomic_store",
            ):
                p = subprocess.run(
                    [
                        "objdump",
                        "--disassemble-all",
                        tmp / f"libute/CMakeFiles/libute.dir/src/{src}.c.o",
                    ],
                    stdout=subprocess.PIPE,
                    check=True,
                    text=True,
                )
                if re.search(r"\b(movdqa|movaps)\b", p.stdout) is None:
                    raise RuntimeError(
                        f"volatile 128-bit load/store in {src}.c "
                        f"was not lowered to MOVDQA/MOVAPS:\n{p.stdout}"
                    )

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
