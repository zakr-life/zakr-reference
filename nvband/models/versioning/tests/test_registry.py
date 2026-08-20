"""Traces: signed registry + firmware-compat gating (CLAUDE.md §4/§6)."""
import json
import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
import registry as reg  # noqa: E402


class TestRegistry(unittest.TestCase):
    def setUp(self):
        self.tmpdir = tempfile.mkdtemp()
        self.artifact_path = os.path.join(self.tmpdir, "artifact.json")
        with open(self.artifact_path, "w") as f:
            json.dump({"dummy": "model"}, f)
        self._orig_registry_path = reg.REGISTRY_PATH
        reg.REGISTRY_PATH = os.path.join(self.tmpdir, "registry.json")

    def tearDown(self):
        reg.REGISTRY_PATH = self._orig_registry_path

    def test_register_and_verify_roundtrip(self):
        entry = reg.register_model("m1", "1.0.0", self.artifact_path, "1.0.0", "2.0.0")
        self.assertTrue(reg.verify_entry_integrity(entry))

    def test_tampered_artifact_fails_verification(self):
        entry = reg.register_model("m1", "1.0.0", self.artifact_path, "1.0.0", "2.0.0")
        with open(self.artifact_path, "a") as f:
            f.write("TAMPERED")
        self.assertFalse(reg.verify_entry_integrity(entry))

    def test_forged_signature_fails_verification(self):
        entry = reg.register_model("m1", "1.0.0", self.artifact_path, "1.0.0", "2.0.0")
        entry.signature = "0" * 64
        self.assertFalse(reg.verify_entry_integrity(entry))

    def test_firmware_compat_range_inclusive_bounds(self):
        entry = reg.register_model("m1", "1.0.0", self.artifact_path, "1.0.0", "1.5.0")
        self.assertTrue(reg.is_firmware_compatible(entry, "1.0.0"))
        self.assertTrue(reg.is_firmware_compatible(entry, "1.5.0"))
        self.assertTrue(reg.is_firmware_compatible(entry, "1.2.3"))
        self.assertFalse(reg.is_firmware_compatible(entry, "0.9.9"))
        self.assertFalse(reg.is_firmware_compatible(entry, "1.5.1"))

    def test_fleet_ota_refuses_out_of_range_firmware(self):
        entry = reg.register_model("m1", "1.0.0", self.artifact_path, "1.0.0", "1.5.0")
        ok, reason = reg.fleet_ota_would_distribute(entry, "9.9.9")
        self.assertFalse(ok)
        self.assertIn("compatibility range", reason)

    def test_fleet_ota_refuses_tampered_artifact_even_if_compatible(self):
        entry = reg.register_model("m1", "1.0.0", self.artifact_path, "1.0.0", "2.0.0")
        with open(self.artifact_path, "a") as f:
            f.write("TAMPERED")
        ok, reason = reg.fleet_ota_would_distribute(entry, "1.5.0")
        self.assertFalse(ok)
        self.assertIn("integrity", reason)

    def test_fleet_ota_distributes_when_compatible_and_untampered(self):
        entry = reg.register_model("m1", "1.0.0", self.artifact_path, "1.0.0", "2.0.0")
        ok, reason = reg.fleet_ota_would_distribute(entry, "1.5.0")
        self.assertTrue(ok)

    def test_invalid_range_rejected_at_registration(self):
        with self.assertRaises(ValueError):
            reg.register_model("m1", "1.0.0", self.artifact_path, "2.0.0", "1.0.0")

    def test_missing_artifact_rejected_at_registration(self):
        with self.assertRaises(FileNotFoundError):
            reg.register_model("m1", "1.0.0", "/nonexistent/path.json", "1.0.0", "2.0.0")


if __name__ == "__main__":
    unittest.main()
