from contextlib import redirect_stdout
from os import PathLike
from io import BytesIO
from pathlib import Path
from sys import modules, executable, argv, version_info
from tempfile import TemporaryDirectory
from traceback import extract_stack
from typing import Optional, TYPE_CHECKING

from IPython import get_ipython
from pkg_resources import working_set
from yaml import safe_load, dump

from tira_measure import MeasurementHandle


if TYPE_CHECKING:
    from ctypes import Pointer
else:
    from typing import Generic, TypeVar

    T = TypeVar("T")

    class Pointer(Generic[T]):
        pass


def persist_ir_metadata(
    measurement_handle: Pointer[MeasurementHandle],
    output_directory: PathLike = Path.cwd(),
    system_name: Optional[str] = None,
    system_description: Optional[str] = None,
) -> None:
    output_directory_path = Path(output_directory)
    if output_directory_path.exists() and not output_directory_path.is_dir():
        raise ValueError("The output directory must be a directory.")
    if not output_directory_path.exists():
        output_directory_path.mkdir(parents=True, exist_ok=True)
    output_file_path = output_directory_path / ".ir-metadata"
    if output_file_path.exists():
        raise ValueError("Metadata file already exists in the output directory.")

    # TODO: Run the C-internal ir_metadata export from the `measurement_handle` to `output_file_path`.

    # Parse the initial ir_metadata.
    with output_file_path.open("rb") as file:
        buffer = file.read()
        buffer.removeprefix(b"ir_metadata.start\n")
        buffer.removesuffix(b"ir_metadata.end\n")
        with BytesIO(buffer) as yaml_file:
            ir_metadata = safe_load(yaml_file)

    # Add user-provided metadata.
    if system_name is not None:
        # TODO: Where should the system_name be stored in the ir_metadata?
        pass

    if system_description is not None:
        # TODO: Where should the system_description be stored in the ir_metadata?
        pass

    # TODO: Add Python-specific metadata to `ir_metadata`.
    python_version = f"{version_info.major}.{version_info.minor}.{version_info.micro}"
    python_executable = executable
    python_arguments = argv
    python_modules = sorted(
        {
            module.split(".")[0]
            for module in modules.keys()
            if not module.startswith("_")
        }
    )
    python_installed_packages = sorted(
        {
            f"{distribution.project_name}=={distribution.version}"
            for distribution in working_set
        }
    )

    ipython = get_ipython()
    python_is_interactive = ipython is not None
    if ipython is not None:
        # TODO: Add IPython-specific metadata to `ir_metadata`.
        with TemporaryDirectory() as temp_dir:
            tmp_dir_path = Path(temp_dir)

            python_script_file_path = tmp_dir_path / "script.py"
            with redirect_stdout(None):
                ipython.magic(f"save -f {python_script_file_path} 1-9999")
            with python_script_file_path.open("rt") as file:
                python_script_file_contents = file.read()

            python_notebook_file_path = tmp_dir_path / "notebook.ipynb"
            with redirect_stdout(None):
                ipython.magic(f"notebook {python_notebook_file_path}")
            with python_notebook_file_path.open("rt") as file:
                python_notebook_file_contents = file.read()

    else:
        # TODO: Add script-specific metadata to `ir_metadata`.
        python_script_file_path = Path(extract_stack()[0].filename).resolve()
        with python_script_file_path.open("rt") as file:
            python_script_file_contents = file.read()

    # Serialize the updated ir_metadata.
    with output_file_path.open("wb") as file:
        file.write(b"ir_metadata.start\n")
        dump(ir_metadata, file)
        file.write(b"ir_metadata.end\n")
