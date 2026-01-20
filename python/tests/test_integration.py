from io import BytesIO
from os import environ
from pathlib import Path
from shutil import copy as shutil_copy
from subprocess import check_output  # nosec
from tempfile import TemporaryDirectory
from zipfile import ZipFile

from yaml import safe_load as yaml_safe_load

_EXAMPLES_DIR_PATH = Path(__file__).parent.parent / "examples"
_EXAMPLE_NOTEBOOK_PATH = _EXAMPLES_DIR_PATH / "example-notebook.ipynb"


def test_jupyter_notebook() -> None:
    with TemporaryDirectory() as temp_dir:
        working_dir_path = Path(temp_dir)

        # Copy the example notebook to the working directory.
        shutil_copy(_EXAMPLE_NOTEBOOK_PATH, working_dir_path / "example-notebook.ipynb")

        # Set the PYTHONPATH environment variable to include the parent directory of the `tirex_tracker` package.
        env = environ.copy()
        if "PYTHONPATH" in env:
            env["PYTHONPATH"] += ":" + str(Path(__file__).parent.parent.resolve())

        export_file_path = working_dir_path / "ir_metadata.yml"
        assert not export_file_path.exists()

        # Run the notebook using the `runnb` command.
        check_output(
            args=["runnb", "--allow-not-trusted", "example-notebook.ipynb"],
            cwd=working_dir_path,
            env=env,
        )  # nosec

        assert export_file_path.exists()

        buffer = Path(export_file_path).read_bytes()
        if buffer.startswith(b"ir_metadata.start\n"):
            buffer = buffer[len(b"ir_metadata.start\n") :]
        if buffer.endswith(b"ir_metadata.end\n"):
            buffer = buffer[: -len(b"ir_metadata.end\n")]

        with BytesIO(buffer) as yaml_file:
            yaml_content = yaml_safe_load(yaml_file)

        assert "implementation" in yaml_content
        assert "executable" in yaml_content["implementation"]
        assert "cmd" in yaml_content["implementation"]["executable"]
        assert isinstance(yaml_content["implementation"]["executable"]["cmd"], list)
        assert "script.py" in yaml_content["implementation"]["executable"]["cmd"]

        assert "source" in yaml_content["implementation"]
        assert "archive" in yaml_content["implementation"]["source"]
        assert "notebook path" in yaml_content["implementation"]["source"]["archive"]
        assert "notebook.ipynb" in yaml_content["implementation"]["source"]["archive"]["notebook path"]
        assert "archive path" in yaml_content["implementation"]["source"]

        archive_path = working_dir_path / Path(yaml_content["implementation"]["source"]["archive path"])
        assert archive_path.exists()

        with ZipFile(archive_path) as zip_file:
            archive_names = zip_file.namelist()
        assert "script.py" in archive_names
        assert "notebook.ipynb" in archive_names
        assert len(archive_names) == 2
