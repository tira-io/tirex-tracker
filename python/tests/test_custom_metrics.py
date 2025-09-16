import unittest
from tempfile import TemporaryDirectory

import yaml
from utils.utils import ChangeWorkingDir

from tirex_tracker import (
    Measure,
    RegisterTIRExInfo,
    deregister_info,
    register_info,
    set_log_callback,
    tracking,
)
from tirex_tracker._custom_metrics import get_info


class CustomMetricTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        set_log_callback(lambda level, component, message: print(f"[{level}][{component}] {message}"))

    @classmethod
    def tearDownClass(cls) -> None:
        set_log_callback(None)

    def testContextManager(self):
        self.assertDictEqual(dict(get_info()), {})
        with RegisterTIRExInfo({"key": "value"}):
            # fetch_info()
            self.assertDictEqual(dict(get_info()), {"key": "value"})
        self.assertDictEqual(dict(get_info()), {})

    def testLowLevelRegistration(self):
        self.assertDictEqual(dict(get_info()), {})
        register_info({"key": "value"})
        self.assertDictEqual(dict(get_info()), {"key": "value"})
        deregister_info(["key"])
        self.assertDictEqual(dict(get_info()), {})

    def testNestedContext(self):
        self.assertDictEqual(dict(get_info()), {})
        with RegisterTIRExInfo({"k1": "v1", "k2": "v2"}):
            self.assertDictEqual(dict(get_info()), {"k1": "v1", "k2": "v2"})
            with RegisterTIRExInfo({"k2": "v3"}):
                self.assertDictEqual(dict(get_info()), {"k1": "v1", "k2": "v3"})
                with RegisterTIRExInfo({"k1": "v4", "k2": "v5"}):
                    self.assertDictEqual(dict(get_info()), {"k1": "v4", "k2": "v5"})
                self.assertDictEqual(dict(get_info()), {"k1": "v1", "k2": "v3"})
            self.assertDictEqual(dict(get_info()), {"k1": "v1", "k2": "v2"})
        self.assertDictEqual(dict(get_info()), {})

    def testPathKeys(self):
        self.assertDictEqual(dict(get_info()), {})
        with RegisterTIRExInfo({"parent": {"child1": "v1", "child2": "v2"}}):
            self.assertDictEqual(dict(get_info()), {"parent": {"child1": "v1", "child2": "v2"}})
            with RegisterTIRExInfo({"parent/child1": "v3"}):
                self.assertDictEqual(dict(get_info()), {"parent": {"child1": "v3", "child2": "v2"}})
            self.assertDictEqual(dict(get_info()), {"parent": {"child1": "v1", "child2": "v2"}})
        self.assertDictEqual(dict(get_info()), {})

    def testIRMetadata(self):
        with TemporaryDirectory() as tmpdir, ChangeWorkingDir(tmpdir):
            with RegisterTIRExInfo({"key": "value"}), tracking(
                measures=[
                    Measure.PYTHON_VERSION,
                    Measure.PYTHON_EXECUTABLE,
                    Measure.PYTHON_ARGUMENTS,
                    Measure.PYTHON_MODULES,
                    Measure.PYTHON_INSTALLED_PACKAGES,
                    Measure.PYTHON_IS_INTERACTIVE,
                    Measure.PYTHON_SCRIPT_FILE_PATH,
                    Measure.PYTHON_NOTEBOOK_FILE_PATH,
                    Measure.PYTHON_CODE_ARCHIVE_PATH,
                    Measure.PYTHON_SCRIPT_FILE_PATH_IN_CODE_ARCHIVE,
                    Measure.PYTHON_NOTEBOOK_FILE_PATH_IN_CODE_ARCHIVE,
                    Measure.TIME_ELAPSED_WALL_CLOCK_MS,
                ],
                export_file_path="./ir_metadata.yml",
            ):
                pass
            with open("./ir_metadata.yml") as file:
                metadata = yaml.safe_load(file)
            print(metadata)
            # TODO: test if the metadata is contained
