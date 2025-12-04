import unittest
from glob import glob
from pathlib import Path
import tempfile

from tirex_tracker import ExportFormat, clear_file_register, register_file, tracking

class TestRedundantTracking(unittest.TestCase):
    def test_with_different_metadata_files(self) -> None:
        expected = ["metadata1.yml", "metadata2.yml"]
        clear_file_register()

        with tempfile.TemporaryDirectory() as tmp:
            with tracking(export_file_path=Path(tmp) / "metadata1.yml", export_format=ExportFormat.IR_METADATA):
                print("hello world")

            with tracking(export_file_path=Path(tmp) / "metadata2.yml", export_format=ExportFormat.IR_METADATA):
                print("hello world")

            actual = sorted([i.split("/")[-1] for i in glob(f"{tmp}/*")])

        self.assertEqual(expected, actual)

    def test_with_identical_metadata_files(self) -> None:
        expected = ["metadata.yml"]
        clear_file_register()

        with tempfile.TemporaryDirectory() as tmp:
            with tracking(export_file_path=Path(tmp) / "metadata.yml", export_format=ExportFormat.IR_METADATA):
                print("hello world")

            with self.assertRaises(ValueError):
                with tracking(export_file_path=Path(tmp) / "metadata.yml", export_format=ExportFormat.IR_METADATA):
                    print("hello world")

            actual = sorted([i.split("/")[-1] for i in glob(f"{tmp}/*")])

        self.assertEqual(expected, actual)
