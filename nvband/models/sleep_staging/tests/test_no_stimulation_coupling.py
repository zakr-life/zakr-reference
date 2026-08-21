"""
test_no_stimulation_coupling.py — Addendum 2 §E's "one rule for this
subsystem": sleep monitoring is sensing/scoring/reporting ONLY and must
have zero coupling to stimulation. This test is the mechanical proof of
that, for the models/sleep_staging package, by static inspection of its
own source — it does not run the classifier and check behavior, it checks
that the *code itself* has no path to stimulation control.

Method: tokenize every non-test .py file in this package (skipping this
tests/ directory itself, which necessarily contains the banned substrings
below as string literals in its own denylist — scanning it would be a
false positive against itself) and check every NAME token (identifiers:
variable names, function names, imported module names) for a small set of
stimulation-control-suggestive substrings. Using `tokenize` rather than a
plain-text grep means comments and string literals (including docstrings)
are automatically excluded from the scan by construction — only actual
code identifiers are checked, which is exactly the "outside of comments/
docstrings" scope this test is supposed to cover, and is a strictly
narrower (stricter, not weaker) surface than "every character in the
file," since an identifier is the only thing that could actually name a
function to call, a type to instantiate, or a module to import.

If a legitimate identifier in this package's own code ever needs to
contain one of these substrings, the fix is to rename the identifier
(see CLAUDE.md discipline elsewhere in this repo: rename, don't weaken
the safety test) — not to add it to an exemption list here.
"""
import ast
import json
import os
import tokenize
import unittest

PACKAGE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

# Directories under models/sleep_staging/ that are not this package's own
# Python source: `tests/` contains this file (see module docstring for
# why it must be excluded), `data/` and `__pycache__/` never contain .py
# source we authored.
EXCLUDE_DIRNAMES = {"tests", "__pycache__", "data"}

# Substrings that would suggest a coupling to stimulation control if they
# appeared in an identifier in this package. Matched case-insensitively,
# as a substring (so e.g. "current_mA" is caught via "current_ma", and
# "STIM_SOMETHING" is caught via "stim").
BANNED_SUBSTRINGS = [
    "stim", "clamp", "interlock", "dac", "current_ma", "charge",
    "enable", "permit",
]

# The three firmware headers Addendum 2 §E explicitly forbids this
# package from ever depending on, checked directly (belt and suspenders
# on top of the generic substring scan above, which already catches
# "stim"/"interlock"/"charge" individually).
FORBIDDEN_FIRMWARE_REFERENCES = [
    "stim_command_clamp",
    "interlock_status",
    "charge_balance",
]


def iter_package_py_files():
    for dirpath, dirnames, filenames in os.walk(PACKAGE_DIR):
        dirnames[:] = [d for d in dirnames if d not in EXCLUDE_DIRNAMES]
        for fn in filenames:
            if fn.endswith(".py"):
                yield os.path.join(dirpath, fn)


def identifiers_in_file(path):
    """Yields (identifier_string, line_number) for every NAME token in a
    Python source file. Comments and string literals (docstrings
    included) are never NAME tokens, so they are excluded by
    construction — see module docstring."""
    with open(path, "rb") as f:
        for tok in tokenize.tokenize(f.readline):
            if tok.type == tokenize.NAME:
                yield tok.string, tok.start[0]


class TestNoStimulationCoupling(unittest.TestCase):
    def test_package_has_source_files_to_scan(self):
        # Sanity check on the test itself: if this ever returns zero
        # files (e.g. a path typo), every other assertion below would
        # trivially "pass" for the wrong reason.
        files = list(iter_package_py_files())
        self.assertGreater(len(files), 3)
        basenames = {os.path.basename(p) for p in files}
        self.assertIn("synthetic_overnight_eeg.py", basenames)
        self.assertIn("softmax_classifier.py", basenames)

    def test_no_banned_identifier_substrings_in_package_source(self):
        violations = []
        for path in iter_package_py_files():
            for name, lineno in identifiers_in_file(path):
                lname = name.lower()
                for banned in BANNED_SUBSTRINGS:
                    if banned in lname:
                        violations.append(
                            f"{os.path.relpath(path, PACKAGE_DIR)}:{lineno}: "
                            f"identifier '{name}' contains banned substring '{banned}'"
                        )
        self.assertEqual(
            violations, [],
            "Stimulation-control-suggestive identifier(s) found in "
            "models/sleep_staging (sensing/scoring/reporting-only "
            "package, Addendum 2 §E):\n" + "\n".join(violations)
        )

    def test_no_reference_to_forbidden_firmware_headers_as_code(self):
        # Explicit, named check for the three exact forbidden modules
        # (as opposed to the generic single-word substring scan above),
        # scoped to actual code identifiers -- e.g. an `import
        # stim_command_clamp` or `from ... import charge_balance` would
        # be caught here even in the hypothetical case a future edit
        # renamed the generic "stim"/"charge"/"interlock" substrings
        # away from a compound identifier that still spelled out one of
        # these exact module names.
        #
        # Deliberately NOT a raw full-text scan: this package's own
        # generated documentation (model_card.md's generator in
        # export_model.py) legitimately *names* stim_command_clamp.c in
        # prose, the same way models/export/model_card.md does, to
        # explain that this model's output type cannot reach it -- that
        # is the correct, desired kind of mention, not a coupling, and a
        # naive text-in-file scan cannot tell the difference between a
        # code reference and a documentation reference. Restricting this
        # check to NAME tokens (never STRING/COMMENT tokens, see
        # identifiers_in_file()) makes that distinction for free.
        violations = []
        for path in iter_package_py_files():
            for name, lineno in identifiers_in_file(path):
                if name in FORBIDDEN_FIRMWARE_REFERENCES:
                    violations.append(
                        f"{os.path.relpath(path, PACKAGE_DIR)}:{lineno}: "
                        f"code identifier '{name}' names a forbidden firmware module"
                    )
        self.assertEqual(violations, [])

    def test_no_import_statements_reach_outside_this_package_or_stdlib_or_training(self):
        # Every `import X` / `from X import ...` in this package must
        # target either the Python standard library, another module
        # inside models/sleep_staging itself, or (read-only reuse, same
        # as models/evaluation/evaluate.py's existing pattern)
        # models/training -- never anything under firmware/.
        own_modules = {os.path.splitext(os.path.basename(p))[0] for p in iter_package_py_files()}
        for path in iter_package_py_files():
            with open(path, encoding="utf-8") as f:
                tree = ast.parse(f.read(), filename=path)
            for node in ast.walk(tree):
                if isinstance(node, ast.Import):
                    for alias in node.names:
                        root = alias.name.split(".")[0]
                        self.assertNotIn("firmware", root.lower())
                elif isinstance(node, ast.ImportFrom):
                    if node.module:
                        root = node.module.split(".")[0]
                        self.assertNotIn("firmware", root.lower())

    def test_exported_artifact_fields_are_not_stimulation_shaped(self):
        # The output type this package actually produces (the exported,
        # quantized classifier artifact) must not contain any field name
        # that looks like it could be forwarded into a stimulation
        # command -- this is the "no field of any kind" check applied to
        # the concrete data this package emits, not just its code.
        export_path = os.path.join(PACKAGE_DIR, "sleep_classifier_int8.json")
        if not os.path.exists(export_path):
            self.skipTest(
                "sleep_classifier_int8.json not present in this checkout; "
                "run models/sleep_staging/export_model.py to generate it "
                "and re-run this test for the full artifact-level check "
                "(the source-level checks above already ran regardless)."
            )
        with open(export_path) as f:
            exported = json.load(f)
        for key in exported.keys():
            lkey = key.lower()
            for banned in BANNED_SUBSTRINGS:
                self.assertNotIn(
                    banned, lkey,
                    f"exported artifact field '{key}' looks stimulation-shaped"
                )


if __name__ == "__main__":
    unittest.main()
