from __future__ import annotations

from contextlib import redirect_stdout
from pathlib import Path
from traceback import extract_stack
from typing import NamedTuple, Optional

from IPython import InteractiveShell, get_ipython


class ZipPaths(NamedTuple):
    script_file_path: Path
    notebook_file_path: Optional[Path]
    script_file_path_in_git_archive: Optional[Path]
    notebook_file_path_in_git_archive: Optional[Path]


def _create_notebook_zip_archive(
    metadata_directory_path: Path,
    ipython: InteractiveShell,
    git_archive_path: Optional[Path],
) -> ZipPaths:
    script_file_path = metadata_directory_path / "script.py"
    notebook_file_path = metadata_directory_path / "notebook.ipynb"
    with redirect_stdout(None):
        ipython.run_line_magic("save", f"-f {script_file_path} 1-9999")
        ipython.run_line_magic("notebook", str(notebook_file_path))

    script_file_path_in_git_archive = Path("script.py")
    notebook_file_path_in_git_archive = Path("notebook.ipynb")
    # TODO: Add the notebook and script files to the Git archive.

    return ZipPaths(
        script_file_path=script_file_path,
        notebook_file_path=notebook_file_path,
        script_file_path_in_git_archive=script_file_path_in_git_archive,
        notebook_file_path_in_git_archive=notebook_file_path_in_git_archive,
    )


def create_code_archive(
    metadata_directory_path: Path,
    git_archive_path: Optional[Path],
    git_root_path: Optional[Path],
) -> ZipPaths:
    ipython = get_ipython()
    if ipython is not None:
        return _create_notebook_zip_archive(
            metadata_directory_path=metadata_directory_path,
            ipython=ipython,
            git_archive_path=git_archive_path,
        )
    else:
        script_file_path = Path(extract_stack()[0].filename).resolve()

        return ZipPaths(
            script_file_path=script_file_path,
            notebook_file_path=None,
            script_file_path_in_git_archive=script_file_path.relative_to(git_root_path)
            if git_root_path is not None
            else None,
            notebook_file_path_in_git_archive=None,
        )
