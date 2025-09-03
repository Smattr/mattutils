#!/usr/bin/env python3

"""
transfer file content via keystrokes

Sometimes you need to transfer a file to a remote machine to which you have no
access beyond a terminal/RDP window into its environment. This script lets you
type a Base64-encoded version of a file directly into a receiving text file or
terminal.
"""

import argparse
import base64
import io
import os
import subprocess
import sys
from pathlib import Path


def type_in(s: str) -> int:
    """type the given string"""

    # choose a typing assistant
    if os.environ.get("XDG_SESSION_TYPE") == "x11":
        dotool = "xdotool"  # X11
    else:
        raise NotImplementedError("no Wayland support")

    args = [dotool, "key"]
    xkeys = {
        "\n": "KP_Enter",
        "-": "KP_Subtract",
        "_": "underscore",
        "=": "KP_Equal",
        "+": "KP_Add",
        ".": "KP_Decimal",
        "/": "KP_Divide",
    }
    for char in s:
        args += [xkeys.get(char, char)]

    return subprocess.call(args)


def main(args: list[str]) -> int:
    """entry point"""

    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument(
        "source",
        default=sys.stdin.buffer,
        nargs="?",
        type=argparse.FileType("rb"),
        help="file content to send",
    )
    options = parser.parse_args(args[1:])

    # Base64 encode the content
    encoded = io.BytesIO()
    base64.encode(options.source, encoded)

    # send the encoded content to the active window
    ret = type_in(encoded.getvalue().decode("utf-8"))

    subprocess.run(["notify-send", Path(__file__).name, "Done!"], check=True)

    return ret


if __name__ == "__main__":
    sys.exit(main(sys.argv))
