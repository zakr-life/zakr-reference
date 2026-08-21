"""Traces: synthetic overnight dataset structure and provenance (Addendum 2 §E)."""
import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from synthetic_overnight_eeg import generate_dataset, STAGES  # noqa: E402


class TestSyntheticOvernightDataset(unittest.TestCase):
    def test_every_epoch_tagged_synthetic(self):
        epochs = generate_dataset(num_subjects=3, nights_per_subject=1, epochs_per_night=20)
        self.assertTrue(all(e.data_provenance == "synthetic" for e in epochs))

    def test_every_stage_label_is_valid(self):
        epochs = generate_dataset(num_subjects=4, nights_per_subject=1, epochs_per_night=40)
        self.assertTrue(all(e.stage in STAGES for e in epochs))

    def test_deterministic_given_seed(self):
        a = generate_dataset(num_subjects=3, nights_per_subject=1, epochs_per_night=10, seed=99)
        b = generate_dataset(num_subjects=3, nights_per_subject=1, epochs_per_night=10, seed=99)
        self.assertEqual([e.stage for e in a], [e.stage for e in b])
        self.assertEqual([e.imu_motion_magnitude for e in a], [e.imu_motion_magnitude for e in b])

    def test_n3_has_lower_average_movement_than_wake(self):
        # Not a strict per-epoch guarantee (both are randomized), but the
        # generator's whole reason for existing is that this holds in
        # aggregate -- this is the one property this test enforces.
        epochs = generate_dataset(num_subjects=10, nights_per_subject=2, epochs_per_night=180)
        n3 = [e.imu_motion_magnitude for e in epochs if e.stage == "N3" and not e.motion_contaminated]
        wake = [e.imu_motion_magnitude for e in epochs if e.stage == "WAKE" and not e.motion_contaminated]
        self.assertTrue(n3, "expected at least one N3 epoch in this sample")
        self.assertTrue(wake, "expected at least one WAKE epoch in this sample")
        self.assertLess(sum(n3) / len(n3), sum(wake) / len(wake))

    def test_subject_and_session_ids_partition_epochs(self):
        epochs = generate_dataset(num_subjects=2, nights_per_subject=2, epochs_per_night=15)
        sessions = {e.session_id for e in epochs}
        self.assertEqual(len(sessions), 4)  # 2 subjects x 2 nights


if __name__ == "__main__":
    unittest.main()
