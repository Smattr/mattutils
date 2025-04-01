#!/usr/bin/env python3

"""
tests for dif.c
"""

import re
import subprocess
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
    diff = (
        b"diff --git a b\n"
        b"--- a\n"
        b"+++ b\n"
        b"@@ -11,6 +11,21 @@\n"
        b"-foo\n"
        b"+ba\x00r"
    )

    subprocess.run(["dif"], input=diff, check=True)


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
