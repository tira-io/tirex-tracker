from contextlib import redirect_stdout
from pathlib import Path
from traceback import extract_stack
from typing import NamedTuple, Iterable, Optional
from zipfile import ZIP_DEFLATED, ZipFile

from git import InvalidGitRepositoryError, Repo, IndexObject
from IPython import InteractiveShell, get_ipython


class ZipPaths(NamedTuple):
    script_file_path: Path
    notebook_file_path: Optional[Path]
    zip_file_path: Path
    script_file_path_in_zip: Path
    notebook_file_path_in_zip: Optional[Path]


def _create_zip_archive(
    metadata_directory_path: Path,
    base_directory_path: Path,
    script_file_path: Path,
    notebook_file_path: Optional[Path],
    other_paths: Iterable[Path] = tuple(),
) -> ZipPaths:
    all_paths = [
        script_file_path,
        *([notebook_file_path] if notebook_file_path else []),
        *other_paths,
    ]
    zip_file_path = metadata_directory_path / "code.zip"
    with ZipFile(zip_file_path, "w", compression=ZIP_DEFLATED) as zip_file:
        for path in all_paths:
            relative_path = path.relative_to(base_directory_path)
            zip_file.write(path, arcname=relative_path)
    return ZipPaths(
        script_file_path=script_file_path,
        notebook_file_path=notebook_file_path,
        zip_file_path=zip_file_path,
        script_file_path_in_zip=script_file_path.relative_to(base_directory_path),
        notebook_file_path_in_zip=(
            notebook_file_path.relative_to(base_directory_path)
            if notebook_file_path
            else None
        ),
    )


def _create_notebook_zip_archive(
    metadata_directory_path: Path,
    ipython: InteractiveShell,
) -> ZipPaths:
    notebook_file_path = metadata_directory_path / "notebook.ipynb"
    script_file_path = metadata_directory_path / "script.py"
    with redirect_stdout(None):
        ipython.magic(f"save -f {script_file_path} 1-9999")
        ipython.magic(f"notebook {notebook_file_path}")
    return _create_zip_archive(
        metadata_directory_path=metadata_directory_path,
        base_directory_path=metadata_directory_path,
        script_file_path=script_file_path,
        notebook_file_path=notebook_file_path,
    )


def create_git_zip_archive(
    metadata_directory_path: Path,
    script_file_path: Path,
) -> Optional[ZipPaths]:
    """
    Creates a ZIP archive containing all tracked files in a given Git repository.

    :param directory_path_in_git_repository: A directory that is inside the Git repository that should be archived.
    """

    try:
        repo = Repo(script_file_path, search_parent_directories=True)
    except InvalidGitRepositoryError:
        return None

    working_tree_dir = repo.working_tree_dir
    if working_tree_dir is None:
        return None
    working_tree_dir_path = Path(working_tree_dir)

    tracked_paths = (
        Path(item.abspath)
        for item in repo.commit().tree.traverse()
        if isinstance(item, IndexObject)
    )
    tracked_file_paths = (
        path
        for path in tracked_paths
        if path.is_file()
    )

    return _create_zip_archive(
        metadata_directory_path=metadata_directory_path,
        base_directory_path=working_tree_dir_path,
        script_file_path=script_file_path,
        notebook_file_path=None,
        other_paths=tracked_file_paths,
    )


def create_code_archive(
    metadata_directory_path: Path,
) -> ZipPaths:
    ipython = get_ipython()
    if ipython is not None:
        return _create_notebook_zip_archive(
            metadata_directory_path=metadata_directory_path,
            ipython=ipython,
        )
    else:
        script_file_path = Path(extract_stack()[0].filename).resolve()

        paths = create_git_zip_archive(
            metadata_directory_path=metadata_directory_path,
            script_file_path=script_file_path,
        )
        if paths is not None:
            return paths
        return _create_zip_archive(
            metadata_directory_path=metadata_directory_path,
            base_directory_path=script_file_path.parent,
            script_file_path=script_file_path,
            notebook_file_path=None,
        )
