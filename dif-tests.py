#!/usr/bin/env python3

"""
tests for dif.c
"""

import re
from pathlib import Path

import pexpect
import pytest


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
    proc.expect(pexpect.EOF)


def test_unicode_prefix(tmp_path: Path):
    """are non-ASCII characters shown correctly when their prefix matches?"""

    a = tmp_path / "a"
    a.write_text("foo ┌─┐\n", encoding="utf-8")

    b = tmp_path / "b"
    b.write_text("foo ┌──┐ \n", encoding="utf-8")

    proc = pexpect.spawn("dif", [str(a), str(b)], encoding="utf-8")
    proc.expect(re.compile(r"^\[31m-.*┌.*┐", flags=re.MULTILINE))
    proc.expect(re.compile(r"^\[32m+.*┌.*┐", flags=re.MULTILINE))
    proc.expect(pexpect.EOF)


def test_unicode_suffix(tmp_path: Path):
    """are non-ASCII characters shown correctly when their suffix matches?"""

    a = tmp_path / "a"
    a.write_text("foo ┌─┐\n", encoding="utf-8")

    b = tmp_path / "b"
    b.write_text("foo ┌──┐\n", encoding="utf-8")

    proc = pexpect.spawn("dif", [str(a), str(b)], encoding="utf-8")
    proc.expect(re.compile(r"^\[31m-.*┌.*┐", flags=re.MULTILINE))
    proc.expect(re.compile(r"^\[32m+.*┌.*┐", flags=re.MULTILINE))
    proc.expect(pexpect.EOF)
