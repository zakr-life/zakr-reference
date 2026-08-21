"""Traces: motion-contaminated epochs never reach the sleep-stage classifier (Addendum 2 §E)."""
import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from features import (reject_motion_contaminated, epoch_to_feature_vector,  # noqa: E402
                       FEATURE_ORDER, BAND_FEATURE_ORDER, label_to_index)
from synthetic_overnight_eeg import generate_dataset, STAGES  # noqa: E402


class TestMotionRejection(unittest.TestCase):
    def test_contaminated_epochs_excluded_from_clean_set(self):
        epochs = generate_dataset(num_subjects=5, nights_per_subject=1,
                                   epochs_per_night=60, contamination_rate=0.3)
        clean, contaminated = reject_motion_contaminated(epochs)

        self.assertTrue(all(not e.motion_contaminated for e in clean))
        self.assertTrue(all(e.motion_contaminated for e in contaminated))
        self.assertEqual(len(clean) + len(contaminated), len(epochs))

    def test_no_epoch_lost(self):
        epochs = generate_dataset(num_subjects=3, nights_per_subject=1,
                                   epochs_per_night=20)
        clean, contaminated = reject_motion_contaminated(epochs)
        self.assertEqual(len(clean) + len(contaminated), len(epochs))

    def test_feature_vector_matches_declared_order_and_length(self):
        epochs = generate_dataset(num_subjects=1, nights_per_subject=1,
                                   epochs_per_night=1)
        vec = epoch_to_feature_vector(epochs[0])
        self.assertEqual(len(vec), len(FEATURE_ORDER))
        # 4 channels x 5 bands + 1 IMU movement feature.
        self.assertEqual(len(BAND_FEATURE_ORDER), 20)
        self.assertEqual(len(FEATURE_ORDER), 21)
        self.assertEqual(FEATURE_ORDER[-1], "imu_motion_magnitude")

    def test_label_to_index_covers_all_five_stages(self):
        self.assertEqual(len(STAGES), 5)
        indices = {label_to_index(s) for s in STAGES}
        self.assertEqual(indices, {0, 1, 2, 3, 4})


if __name__ == "__main__":
    unittest.main()
