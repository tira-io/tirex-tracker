import unittest
import tempfile
import os
from pathlib import Path
import subprocess

def run_command_in_docker_image(command, docker_image):
    with tempfile.TemporaryDirectory(delete=False) as tmp_dir:
        tracker_dir = str(Path(__file__).parent.parent.absolute())
        subprocess.run(["docker", "run", "-e", "TIRA_OUTPUT_DIR=/fooo", "--rm", "-v", f"{tmp_dir}:/fooo", "-v", f"{tracker_dir}:/tirex-tracker:ro", "--entrypoint", "/tirex-tracker/tirex-tracker.sh", docker_image, command])
        return tmp_dir


class TestInDockerImagesWithTirexTrackerAvailable(unittest.TestCase):
    def test_bash_image_01(self):
        expected_files = sorted(["foo", "tira-ir-metadata.yml"])
        result_dir = run_command_in_docker_image("echo 'hallo welt' > ${TIRA_OUTPUT_DIR}/foo", "ubuntu:22.04")
        actual = sorted(os.listdir(result_dir))
        self.assertEqual(expected_files, actual)

        actual_text = (Path(result_dir) / "foo").read_text()
        self.assertEqual("hallo welt\n", actual_text)

        actual_metadata = (Path(result_dir) / "tira-ir-metadata.yml").read_text()
        self.assertIn("schema version:", actual_metadata)

    def test_bash_image_02(self):
        expected_files = sorted(["tmp", "tira-ir-metadata.yml"])
        result_dir = run_command_in_docker_image("echo 'foo-bar' > ${TIRA_OUTPUT_DIR}/tmp", "ubuntu:22.04")
        actual = sorted(os.listdir(result_dir))
        self.assertEqual(expected_files, actual)

        actual_text = (Path(result_dir) / "tmp").read_text()
        self.assertEqual("foo-bar\n", actual_text)

        actual_metadata = (Path(result_dir) / "tira-ir-metadata.yml").read_text()
        self.assertIn("schema version:", actual_metadata)
    
