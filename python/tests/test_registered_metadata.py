import tempfile
import unittest
from pathlib import Path

import yaml

from tirex_tracker import ExportFormat, clear_metadata_register, register_metadata, tracking


class TestRegisteredMetadata(unittest.TestCase):
    def test_without_registered_value(self) -> None:
        clear_metadata_register()
        with tempfile.TemporaryDirectory() as tmp:
            result_path = Path(tmp) / "example-metadata.yml"
            with tracking(export_file_path=result_path, export_format=ExportFormat.IR_METADATA):
                print("hello world")

            actual = yaml.safe_load(result_path.read_text())
            self.assertIn("implementation", actual)
            self.assertNotIn("data", actual)

    def test_with_registered_value(self) -> None:
        clear_metadata_register()
        register_metadata({"data": {"test collection": {"name": "some-dataset"}}})
        with tempfile.TemporaryDirectory() as tmp:
            result_path = Path(tmp) / "example-metadata.yml"
            with tracking(export_file_path=result_path, export_format=ExportFormat.IR_METADATA):
                print("hello world")

            actual = yaml.safe_load(result_path.read_text())
            self.assertIn("implementation", actual)
            self.assertIn("data", actual)
