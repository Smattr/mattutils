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


@pytest.mark.xfail(strict=True)
def test_create_with_space():
    """can `dif` handle a file with a space in the name being created?"""
    diff = textwrap.dedent(
        """\
    diff --git hello world hello world
    new file mode 100644
    index 0000000..ce01362
    --- /dev/null
    +++ hello world 
    @@ -0,0 +1 @@
    +foo bar
    """
    )

    proc = pexpect.spawn("dif")
    proc.send(diff)
    proc.sendeof()
    proc.wait()
    proc.close()
    assert proc.signalstatus is None, "`dif` crashed when processing create"


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

    proc = pexpect.spawn(
        "dif",
        [str(a), str(b)],
        timeout=1,
        echo=False,
        encoding="utf-8",
        dimensions=(20, 1000),
    )
    did_not_see = proc.expect([re.compile(f"{a} → {b}"), pexpect.EOF])
    assert not did_not_see, "missing git diff header was not correctly handled"


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
    """does `dif` ignore “rename …” lines when not piped?"""

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
    proc = pexpect.spawn("dif", timeout=1, echo=False)
    proc.send(diff)
    proc.sendeof()

    # did we see a rename line?
    did_not_see = proc.expect(["rename", pexpect.EOF])
    assert did_not_see, "rename lines were not suppressed"


def test_rename_retain():
    """does `dif` retain “rename …” lines when piped?"""

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
        re.search(r"\brename\b", proc.stdout) is not None
    ), "rename lines were not retained"


def test_moved_ignore():
    """does `dif` output “… moved and …” lines?"""

    # a diff that indicates a rename with modification
    diff = textwrap.dedent(
        """\
    diff --git foo quux
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

    # run this through `dif`
    proc = pexpect.spawn("dif", timeout=1, echo=False)
    proc.send(diff)
    proc.sendeof()

    # did we see a “… moved and …” line?
    did_not_see = proc.expect(["moved and", pexpect.EOF])
    assert did_not_see, "“moved and” was included"


@pytest.mark.parametrize("similarity", (42, 100))
def test_similarity_ignore(similarity: int):
    """does `dif` ignore “similarity …” lines?"""

    # a diff that indicates a rename with similarity
    diff = textwrap.dedent(
        f"""\
    diff --git foo quux
    similarity index {similarity}%
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

    # run this through `dif`
    proc = pexpect.spawn("dif", timeout=1, echo=False)
    proc.send(diff)
    proc.sendeof()

    # did we see a “similarity …” line?
    did_not_see = proc.expect(["similarity index", pexpect.EOF])
    assert did_not_see, "similarity index was retained"


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
