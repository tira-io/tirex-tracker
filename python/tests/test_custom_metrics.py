import unittest
from tempfile import TemporaryDirectory

import yaml
from utils.utils import ChangeWorkingDir

from tirex_tracker import Measure, RegisterTIRExInfo, deregisterInfo, fetch_info, registerInfo, tracking


class CustomMetricTestCase(unittest.TestCase):
    def testContextManager(self):
        with RegisterTIRExInfo({"key": "value"}):
            fetch_info()

    def testLowLevelRegistration(self):
        registerInfo({"key": "value"})
        fetch_info()
        deregisterInfo(["key"])

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
