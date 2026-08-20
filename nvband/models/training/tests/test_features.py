"""Traces: motion-contaminated epochs never reach the classifier (CLAUDE.md §4 Task 1)."""
import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from features import reject_motion_contaminated, epoch_to_feature_vector, FEATURE_ORDER  # noqa: E402
from synthetic_eeg import generate_dataset  # noqa: E402


class TestMotionRejection(unittest.TestCase):
    def test_contaminated_epochs_excluded_from_clean_set(self):
        epochs = generate_dataset(num_subjects=5, sessions_per_subject=1,
                                   epochs_per_session=50, contamination_rate=0.3)
        clean, contaminated = reject_motion_contaminated(epochs)

        self.assertTrue(all(not e.motion_contaminated for e in clean))
        self.assertTrue(all(e.motion_contaminated for e in contaminated))
        self.assertEqual(len(clean) + len(contaminated), len(epochs))

    def test_no_epoch_lost(self):
        epochs = generate_dataset(num_subjects=3, sessions_per_subject=1,
                                   epochs_per_session=20)
        clean, contaminated = reject_motion_contaminated(epochs)
        self.assertEqual(len(clean) + len(contaminated), len(epochs))

    def test_feature_vector_matches_declared_order_and_length(self):
        epochs = generate_dataset(num_subjects=1, sessions_per_subject=1,
                                   epochs_per_session=1)
        vec = epoch_to_feature_vector(epochs[0])
        self.assertEqual(len(vec), len(FEATURE_ORDER))
        self.assertEqual(len(FEATURE_ORDER), 20)  # 4 channels x 5 bands


if __name__ == "__main__":
    unittest.main()
