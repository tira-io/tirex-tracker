import unittest
from tirex_tracker import tracking, ExportFormat, register_file, clear_file_register
import tempfile
from pathlib import Path
from glob import glob

class TestRegisteredMetadata(unittest.TestCase):
    def test_without_registered_value(self):
        expected = ["metadata.yml"]
        clear_file_register()

        with tempfile.TemporaryDirectory() as tmp:
            with tracking(export_file_path=Path(tmp)/"metadata.yml", export_format=ExportFormat.IR_METADATA):
                print("hello world")
            actual = [i.split('/')[-1] for i in glob(f"{tmp}/*")]

        self.assertEqual(expected, actual)

    def test_with_registered_value(self):
        expected = set(["metadata.yml", "test_integration.py"])
        clear_file_register()
        register_file(Path(__file__).parent, Path("test_integration.py"))

        with tempfile.TemporaryDirectory() as tmp:
            with tracking(export_file_path=Path(tmp)/"metadata.yml", export_format=ExportFormat.IR_METADATA):
                print("hello world")
            actual = set([i.split('/')[-1] for i in glob(f"{tmp}/*")])

        self.assertEqual(expected, actual)

    def test_with_registered_value_to_subdir(self):
        expected = set(["test_integration.py"])
        clear_file_register()
        register_file(Path(__file__).parent, Path("test_integration.py"), "subdir")

        with tempfile.TemporaryDirectory() as tmp:
            with tracking(export_file_path=Path(tmp)/"metadata.yml", export_format=ExportFormat.IR_METADATA):
                print("hello world")
            actual = set([i.split('/')[-1] for i in glob(f"{tmp}/subdir/*")])

        self.assertEqual(expected, actual)
