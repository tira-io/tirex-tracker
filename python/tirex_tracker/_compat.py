from importlib_metadata import PackageNotFoundError, distribution
from typing_extensions import TypeAlias  # type: ignore[import]


def _is_installed(distribution_name: str) -> bool:
    try:
        distribution(distribution_name=distribution_name)
        return True
    except PackageNotFoundError:
        return False


CODECARBON_INSTALLED = _is_installed("codecarbon")
if CODECARBON_INSTALLED:
    from codecarbon import EmissionsTracker as EmissionsTracker  # type: ignore
    from codecarbon.emissions_tracker import BaseEmissionsTracker as BaseEmissionsTracker  # type: ignore
else:
    EmissionsTracker: TypeAlias = NotImplemented  # type: ignore
    BaseEmissionsTracker: TypeAlias = NotImplemented  # type: ignore
