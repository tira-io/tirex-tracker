from pathlib import Path
from tempfile import TemporaryDirectory
from zipfile import ZipFile

from tirex_tracker.archive_utils import create_git_zip_archive

_EXAMPLE_GIT_REPOSITORY_PATH = Path(__file__).parent / "resources" / "example-git-repositories.zip"


def test_code_extraction_from_git_repository() -> None:
    expected_code_files = {
        "some-directory/.gitignore",
        "some-directory/Dockerfile",
        "some-directory/script.sh",
    }

    with TemporaryDirectory() as temp_working_dir:
        working_directory_path = Path(temp_working_dir)

        # Extract the example Git repository to the working directory.
        with ZipFile(_EXAMPLE_GIT_REPOSITORY_PATH, "r") as zip_file:
            zip_file.extractall(working_directory_path)

        script_file_path = working_directory_path / "git-repo-clean" / "some-directory" / "script.sh"
        script_file_path.chmod(0o0766)  # nosec

        with TemporaryDirectory() as tmp_metadata_dir:
            metadata_dir_path = Path(tmp_metadata_dir)

            actual = create_git_zip_archive(
                metadata_directory_path=metadata_dir_path,
                script_file_path=script_file_path,
            )

            assert actual is not None
            assert actual.zip_file_path.exists()

            with ZipFile(actual.zip_file_path) as zip_file:
                archive_names = zip_file.namelist()

            assert sorted(archive_names) == sorted(expected_code_files)


def test_code_extraction_from_git_repository_02() -> None:
    expected_code_files = {
        "some-directory/.gitignore",
        "some-directory/Dockerfile",
        "some-directory/script.sh",
    }

    with TemporaryDirectory() as temp_working_dir:
        working_directory_path = Path(temp_working_dir)

        # Extract the example Git repository to the working directory.
        with ZipFile(_EXAMPLE_GIT_REPOSITORY_PATH, "r") as zip_file:
            zip_file.extractall(working_directory_path)

        script_file_path = (
            working_directory_path / "git-repo-clean-with-additional-content" / "some-directory" / "script.sh"
        )
        script_file_path.chmod(0o0766)  # nosec

        with TemporaryDirectory() as tmp_metadata_dir:
            metadata_dir_path = Path(tmp_metadata_dir)

            actual = create_git_zip_archive(
                metadata_directory_path=metadata_dir_path,
                script_file_path=script_file_path,
            )

            assert actual is not None
            assert actual.zip_file_path.exists()

            with ZipFile(actual.zip_file_path) as zip_file:
                archive_names = zip_file.namelist()

            assert sorted(archive_names) == sorted(expected_code_files)
