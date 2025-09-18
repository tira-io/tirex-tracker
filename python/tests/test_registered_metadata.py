import unittest
from tirex_tracker import tracking, ExportFormat, register_metadata, clear_metadata_register
import tempfile
from pathlib import Path
import yaml

class TestRegisteredMetadata(unittest.TestCase):
    def test_without_registered_value(self):
        clear_metadata_register()
        with tempfile.TemporaryDirectory() as tmp:
            result_path = Path(tmp)/"example-metadata.yml" 
            with tracking(export_file_path=result_path, export_format=ExportFormat.IR_METADATA):
                print("hello world")

            actual = yaml.safe_load(result_path.read_text())
            self.assertIn("implementation", actual)
            self.assertNotIn("data", actual)

    def test_with_registered_value(self):
        clear_metadata_register()
        register_metadata({"data": {"test collection": {"name": "some-dataset"}}})
        with tempfile.TemporaryDirectory() as tmp:
            result_path = Path(tmp)/"example-metadata.yml" 
            with tracking(export_file_path=result_path, export_format=ExportFormat.IR_METADATA):
                print("hello world")

            actual = yaml.safe_load(result_path.read_text())
            self.assertIn("implementation", actual)
            self.assertIn("data", actual)