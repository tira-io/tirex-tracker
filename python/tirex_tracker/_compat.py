from typing import TYPE_CHECKING

from importlib_metadata import PackageNotFoundError, metadata
from typing_extensions import TypeAlias  # type: ignore[import]


def _is_installed(distribution_name: str) -> bool:
    try:
        metadata(distribution_name=distribution_name)
        return True
    except PackageNotFoundError:
        return False


CODECARBON_INSTALLED = _is_installed("codecarbon")
if CODECARBON_INSTALLED or TYPE_CHECKING:
    from codecarbon import EmissionsTracker as EmissionsTracker  # type: ignore[redefine]
    from codecarbon.emissions_tracker import BaseEmissionsTracker as BaseEmissionsTracker  # type: ignore[redefine]
else:
    EmissionsTracker: TypeAlias = NotImplemented  # type: ignore
    BaseEmissionsTracker: TypeAlias = NotImplemented  # type: ignore
