#!/usr/bin/env python3
"""
check_no_stim_coupling.py — Addendum 2 §E's "one rule for this
subsystem" (sleep monitoring is sensing/scoring/reporting only, zero
stimulation authority), enforced mechanically for the C side of the
sleep/ firmware module by static inspection.

Scans every .c/.h file in this directory's parent (`sleep/`, NOT this
tests/ subdirectory — see below) for identifiers that would suggest
stimulation coupling. Comments (`//`, `/* */`) and string/char literals
are stripped before scanning, so a legitimate prose mention (e.g. this
module's own header comment explaining it must never gain a
stimulation-related field) never trips the check — only actual code
identifiers do. This mirrors exactly the design of
`models/sleep_staging/tests/test_no_stimulation_coupling.py`'s
token-based scan, adapted to C since Python's `tokenize` module doesn't
apply here.

This tests/ subdirectory's own source (this file and
test_overnight_session_config.c) is excluded from the scan for the same
reason models/sleep_staging/tests/ is excluded from its Python
counterpart: this file's own denylist necessarily contains these
substrings as string literals, and test_overnight_session_config.c's own
comments legitimately discuss them (see its structural-check test).

Exit code 0 and prints PASS on success; prints every violation and exits
1 otherwise. Invoked by `make test` in this directory's Makefile.
"""
import os
import re
import sys

SLEEP_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

BANNED_SUBSTRINGS = [
    "stim", "clamp", "interlock", "dac", "current_ma", "charge",
    "enable", "permit",
]

FORBIDDEN_INCLUDE_TARGETS = [
    "stim_command_clamp.h",
    "interlock_status.h",
    "charge_balance.h",
]

_COMMENT_OR_STRING_RE = re.compile(
    r"/\*.*?\*/"       # block comments
    r"|//[^\n]*"       # line comments
    r'|"(?:\\.|[^"\\])*"'   # string literals
    r"|'(?:\\.|[^'\\])*'",  # char literals
    re.DOTALL,
)

_IDENTIFIER_RE = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")


def strip_comments_and_literals(text: str) -> str:
    return _COMMENT_OR_STRING_RE.sub(lambda m: " " * len(m.group(0)), text)


def iter_source_files():
    for fn in sorted(os.listdir(SLEEP_DIR)):
        path = os.path.join(SLEEP_DIR, fn)
        if os.path.isfile(path) and (fn.endswith(".c") or fn.endswith(".h")):
            yield path


def main() -> int:
    violations = []
    files = list(iter_source_files())

    if not files:
        print("FAIL: no .c/.h source files found to scan in "
              f"{SLEEP_DIR} -- check the path, an empty scan is not a pass")
        return 1

    for path in files:
        with open(path, encoding="utf-8") as f:
            raw = f.read()
        code_only = strip_comments_and_literals(raw)

        for lineno, line in enumerate(code_only.splitlines(), start=1):
            for m in _IDENTIFIER_RE.finditer(line):
                token = m.group(0).lower()
                for banned in BANNED_SUBSTRINGS:
                    if banned in token:
                        violations.append(
                            f"{os.path.relpath(path, SLEEP_DIR)}:{lineno}: "
                            f"identifier '{m.group(0)}' contains banned "
                            f"substring '{banned}'"
                        )

        # Explicit #include check, independent of the generic scan above
        # (belt and suspenders on the three exact forbidden headers named
        # in the task spec and in overnight_session_config.h's own
        # comment) -- also scoped to code_only so a comment mentioning
        # these filenames does not trip it.
        for m in re.finditer(r'#\s*include\s*"([^"]+)"', code_only):
            included = m.group(1)
            for forbidden in FORBIDDEN_INCLUDE_TARGETS:
                if included.endswith(forbidden):
                    violations.append(
                        f"{os.path.relpath(path, SLEEP_DIR)}: "
                        f"#include \"{included}\" references a forbidden "
                        f"stimulation-related header"
                    )

    if violations:
        print("FAIL: stimulation-coupling-suggestive code found in "
              "firmware/core1_inference_radio/sleep/ "
              "(sensing/scoring/reporting-only module, Addendum 2 §E):")
        for v in violations:
            print(f"  - {v}")
        return 1

    print(f"PASS: scanned {len(files)} file(s) in {os.path.relpath(SLEEP_DIR)}, "
          "no stimulation-coupling-suggestive identifiers or forbidden "
          "#includes found")
    return 0


if __name__ == "__main__":
    sys.exit(main())
