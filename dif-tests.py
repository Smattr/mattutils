#!/usr/bin/env python3

"""
tests for dif.c
"""

import re
import subprocess
import textwrap
from pathlib import Path

import pexpect
import pytest


def test_embedded_nul():
    """how does `dif` react to a `\0` byte in the content?"""
    diff = (
        b"diff --git a b\n"
        b"--- a\n"
        b"+++ b\n"
        b"@@ -11,6 +11,21 @@\n"
        b"-foo\n"
        b"+ba\x00r\n"
    )

    subprocess.run(["dif"], input=diff, check=True)


def test_missing_newline():
    """how does `dif` react to a missing final end line?"""
    diff = textwrap.dedent(
        """\
    diff --git a b
    --- a
    +++ b
    @@ -11,6 +11,21 @@
    -foo
    +bar"""
    )

    subprocess.run(["dif"], input=diff, check=True, text=True)


def test_moved():
    """`dif` should not crash on encountering moved files"""

    diff = textwrap.dedent(
        """\
    diff --git foo quux
    similarity index 74%
    rename from foo
    rename to quux
    index a168ae3..e3c2802 100644
    --- foo
    +++ quux
    @@ -1,4 +1,4 @@
    -hello
    +world
     bar
     baz
     baz
    """
    )

    proc = pexpect.spawn("dif")
    proc.send(diff)
    proc.sendeof()
    proc.wait()
    proc.close()
    assert proc.signalstatus is None, "`dif` crashed when processing move"


@pytest.mark.xfail(strict=True)
def test_no_git_diff_header(tmp_path: Path):
    """does `dif` correctly highlight something not coming from `git`?"""

    a = tmp_path / "a"
    a.write_text("foo\n", encoding="utf-8")

    b = tmp_path / "b"
    b.write_text("bar\n", encoding="utf-8")

    proc = pexpect.spawn("dif", [str(a), str(b)], encoding="utf-8")
    proc.expect(re.compile(f"{a} → {b}"))


def test_plus_plus(tmp_path: Path):
    """
    are lines beginning with e.g. `++[^+]` correctly detected as not part of the header?
    """
    a = tmp_path / "a"
    a.write_text("-foo\n", encoding="utf-8")

    b = tmp_path / "b"
    b.write_text("++bar\n", encoding="utf-8")

    proc = pexpect.spawn("dif", [str(a), str(b)], encoding="utf-8")
    proc.expect(re.compile(r"^\[32m++", flags=re.MULTILINE))


def test_rename_ignore():
    """does `dif` ignore “rename …” lines?"""

    # a diff that indicates a rename
    diff = textwrap.dedent(
        """\
    diff --git a b
    similarity index 100%
    rename from a
    rename to b
    """
    )

    # run this through `dif`
    proc = subprocess.run(
        ["dif"], input=diff, stdout=subprocess.PIPE, check=True, text=True
    )

    assert (
        re.search(r"\brename\b", proc.stdout) is None
    ), "rename lines were not suppressed"


def test_unicode_prefix(tmp_path: Path):
    """are non-ASCII characters shown correctly when their prefix matches?"""

    a = tmp_path / "a"
    a.write_text("foo ┌─┐\n", encoding="utf-8")

    b = tmp_path / "b"
    b.write_text("foo ┌──┐ \n", encoding="utf-8")

    proc = pexpect.spawn("dif", [str(a), str(b)], encoding="utf-8")
    proc.expect(re.compile(r"^\[31m-.*┌.*┐", flags=re.MULTILINE))
    proc.expect(re.compile(r"^\[32m+.*┌.*┐", flags=re.MULTILINE))


def test_unicode_suffix(tmp_path: Path):
    """are non-ASCII characters shown correctly when their suffix matches?"""

    a = tmp_path / "a"
    a.write_text("foo ┌─┐\n", encoding="utf-8")

    b = tmp_path / "b"
    b.write_text("foo ┌──┐\n", encoding="utf-8")

    proc = pexpect.spawn("dif", [str(a), str(b)], encoding="utf-8")
    proc.expect(re.compile(r"^\[31m-.*┌.*┐", flags=re.MULTILINE))
    proc.expect(re.compile(r"^\[32m+.*┌.*┐", flags=re.MULTILINE))
