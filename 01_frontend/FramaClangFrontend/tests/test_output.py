#!/usr/bin/env python3
"""防止固定结果更新时丢失上一份通过版本或覆盖显式输出。"""

import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from experiment.output import output_directory


class OutputTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="buddy-output-policy-")
        self.addCleanup(temporary.cleanup)
        self.path = Path(temporary.name) / "current"
        self.path.mkdir()
        self.write("converted", "old")

    def write(self, status, content):
        (self.path / "summary.json").write_text(json.dumps({"status": status}))
        (self.path / "converted.c").write_text(content)

    def test_success_replaces_without_accumulating_backups(self):
        with output_directory(self.path, replace=True):
            self.write("converted", "new")
        self.assertEqual((self.path / "converted.c").read_text(), "new")
        self.assertFalse(self.path.with_name(".current.previous").exists())

    def test_failed_or_native_only_run_restores_verified_output(self):
        for status in ["failed", "conversion_blocked", "native_checked"]:
            with self.subTest(status=status):
                with output_directory(self.path, replace=True):
                    self.write(status, "candidate")
                self.assertEqual((self.path / "converted.c").read_text(), "old")

    def test_exception_restores_output_and_first_failure_leaves_no_result(self):
        with self.assertRaisesRegex(RuntimeError, "interrupted"):
            with output_directory(self.path, replace=True):
                raise RuntimeError("interrupted")
        self.assertEqual((self.path / "converted.c").read_text(), "old")
        missing = self.path.parent / "first"
        with output_directory(missing, replace=True):
            pass
        self.assertFalse(missing.exists())

    def test_explicit_and_unmanaged_directories_are_not_replaced(self):
        with self.assertRaisesRegex(ValueError, "new or empty"):
            with output_directory(self.path, replace=False):
                self.fail("must reject existing output")
        (self.path / "summary.json").unlink()
        with self.assertRaisesRegex(ValueError, "unmanaged"):
            with output_directory(self.path, replace=True):
                self.fail("must reject unmanaged content")
        self.assertEqual((self.path / "converted.c").read_text(), "old")


if __name__ == "__main__":
    unittest.main()
