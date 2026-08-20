#!/usr/bin/env python3
"""
run_bench_validation.py — Phantom-driven bench validation orchestrator.

CLAUDE.md §2: "tools/bench-test/ — phantom-driven bench validation
scripts (explicitly non-production)."

**This tool is explicitly non-production** (CLAUDE.md §0.1, §9): it
drives the resistive head-phantom simulator (firmware/sim/) through the
same fault-injection/scenario suite CI runs, and produces a
timestamped, human-readable bench report — the kind of artifact a
manufacturing-line or bring-up bench technician would file per unit
under test. It has and must never gain any code path that talks to real
stimulation hardware without the full hardware interlock chain in place;
on a real bench rig, the phantom in firmware/sim/ is what stands in for
a person while validating firmware behavior, and this script only
ever orchestrates that simulation plus reports its result — it does not
itself implement or bypass any safety check.

Usage: python3 run_bench_validation.py [--unit-id UNIT_ID]
"""
import argparse
import datetime
import json
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
FIRMWARE_DIR = REPO_ROOT / "firmware"


def run_host_test_suite():
    """Runs the full firmware host test suite (includes firmware/sim's
    phantom-driven scenarios) and captures pass/fail per stage."""
    script = FIRMWARE_DIR / "run_host_tests.sh"
    result = subprocess.run(
        ["bash", str(script)], cwd=str(FIRMWARE_DIR),
        capture_output=True, text=True, timeout=300,
    )
    return {
        "returncode": result.returncode,
        "passed": result.returncode == 0,
        "stdout_tail": result.stdout[-4000:],
        "stderr_tail": result.stderr[-2000:],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unit-id", default="BENCH-DEV-UNIT",
                         help="Bench unit identifier for the report filename "
                              "(NOT a manufacturing serial — this is a "
                              "software-only bench run, see tools/provisioning "
                              "for the real per-unit manufacturing record chain)")
    args = parser.parse_args()

    print(f"== NV-Band bench validation (NON-PRODUCTION) for {args.unit_id} ==")
    print("This run validates FIRMWARE BEHAVIOR against the resistive "
          "phantom simulator. It is not a HiPot test, not a biocompatibility "
          "test, and not a substitute for any manufacturing-line electrical "
          "safety gate (see tools/provisioning/).")

    suite_result = run_host_test_suite()

    report = {
        "tool": "tools/bench-test/run_bench_validation.py",
        "non_production": True,
        "unit_id": args.unit_id,
        "timestamp_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "firmware_host_test_suite": suite_result,
        "disclaimer": (
            "This report does not constitute IEC 60601-1 electrical safety "
            "testing, biocompatibility testing, or clinical validation. It "
            "verifies firmware software behavior against a simulated "
            "resistive phantom only. See CLAUDE.md §9, §10, OI-5."
        ),
    }

    reports_dir = Path(__file__).parent / "reports"
    reports_dir.mkdir(exist_ok=True)
    ts = report["timestamp_utc"].replace(":", "-")
    report_path = reports_dir / f"bench_report_{args.unit_id}_{ts}.json"
    with open(report_path, "w") as f:
        json.dump(report, f, indent=2)

    status = "PASS" if suite_result["passed"] else "FAIL"
    print(f"\nfirmware host test suite: {status}")
    print(f"wrote {report_path}")

    sys.exit(0 if suite_result["passed"] else 1)


if __name__ == "__main__":
    main()
