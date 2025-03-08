import os
import tempfile
import unittest
from pathlib import Path
from zipfile import ZipFile

from tirex_tracker import zip_code


class TestExtractionOfCode(unittest.TestCase):

    def test_code_extraction_from_git_repository(self):
        expected_code_files = ["some-directory/.gitignore", "some-directory/Dockerfile", "some-directory/script.sh"]

        with tempfile.TemporaryDirectory() as tmp_file:
            with ZipFile(Path("test") / "resources" / "example-git-repositories.zip", "r") as zip_ref:
                zip_ref.extractall(tmp_file)

            os.chmod(str(Path(tmp_file) / "git-repo-clean" / "some-directory" / "script.sh"), 0o0766)
            actual = zip_code(Path(tmp_file) / "git-repo-clean" / "some-directory")

        zipObj = ZipFile(actual)
        files_in_zip = [i.filename for i in zipObj.infolist()]

        self.assertEqual(files_in_zip, expected_code_files)
