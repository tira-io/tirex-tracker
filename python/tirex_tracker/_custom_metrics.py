from __future__ import annotations

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from typing import Iterable


def registerInfo(info: dict[str, str]) -> None:
    """_summary_

    Examples:
        >>> registerInfo({"hello": "world", "foo": "bar"})
        >>> deregisterInfo(["hello", "foo"])
    """
    pass


def deregisterInfo(keys: Iterable[str]) -> None:
    pass


class RegisterTIRExInfo:
    """_summary_

    Examples:
        >>> with RegisterTIRExInfo({"/data/test collection/ir_datasets": "trec-2019-train", "foo": "bar"}):
        >>>   pass  # Do tracking here and save to ir-datasets file
    """

    def __init__(self, info: dict[str, str]) -> None:
        self._info = info

    def __enter__(self) -> None:
        registerInfo(self._info)

    def __exit__(self, exc_type, exc_val, exc_tb) -> None:
        deregisterInfo(self._info.keys())
