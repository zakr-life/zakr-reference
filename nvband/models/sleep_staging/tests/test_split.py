"""Traces: sleep-staging pipeline subject-level split leakage guarantee (Addendum 2 §E)."""
import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from split import split_subjects, apply_split, assert_no_subject_leakage  # noqa: E402
from synthetic_overnight_eeg import generate_dataset  # noqa: E402


class TestSubjectLevelSplit(unittest.TestCase):
    def test_no_subject_appears_in_two_buckets(self):
        subjects = [f"S{i}" for i in range(20)]
        split = split_subjects(subjects)
        assert_no_subject_leakage(split)  # must not raise

    def test_detects_injected_leakage(self):
        split = {"train": ["A", "B"], "val": ["B", "C"], "test": ["D"]}
        with self.assertRaises(AssertionError):
            assert_no_subject_leakage(split)

    def test_every_subject_assigned_exactly_once(self):
        subjects = [f"S{i}" for i in range(30)]
        split = split_subjects(subjects)
        all_assigned = split["train"] + split["val"] + split["test"]
        self.assertEqual(sorted(all_assigned), sorted(subjects))

    def test_apply_split_partitions_epochs_by_subject_not_epoch(self):
        epochs = generate_dataset(num_subjects=6, nights_per_subject=1,
                                   epochs_per_night=10)
        subject_ids = sorted(set(e.subject_id for e in epochs))
        split = split_subjects(subject_ids, train_frac=0.5, val_frac=0.166)
        buckets = apply_split(epochs, split)

        subject_to_bucket = {}
        for b, subs in split.items():
            for s in subs:
                subject_to_bucket[s] = b
        for bucket, epoch_list in buckets.items():
            for e in epoch_list:
                self.assertEqual(subject_to_bucket[e.subject_id], bucket)

    def test_apply_split_rejects_unknown_subject(self):
        epochs = generate_dataset(num_subjects=2, nights_per_subject=1,
                                   epochs_per_night=4)
        with self.assertRaises(ValueError):
            apply_split(epochs, {"train": ["NOT-A-REAL-SUBJECT"], "val": [], "test": []})


if __name__ == "__main__":
    unittest.main()
