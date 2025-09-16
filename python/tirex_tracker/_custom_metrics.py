from __future__ import annotations

from typing import TYPE_CHECKING

from ._utils.deepchainmap import DeepChainMap

if TYPE_CHECKING:
    from typing import Iterable, Mapping, Union

    VALUE = Union[Mapping[str, "VALUE"], str]

__info: DeepChainMap[str, VALUE] = DeepChainMap()


def register_info(info: dict[str, VALUE]) -> None:
    """TODO: summary

    Examples:
        >>> registerInfo({"hello": "world", "foo": "bar"})
        >>> deregisterInfo(["hello", "foo"])
    """
    # TODO: recursively replace dicts in the value with DeepChainMap
    __info.push(info)


def deregister_info(keys: Iterable[str]) -> None:
    for key in keys:
        del __info[key]


class RegisterTIRExInfo:
    """TODO: summary

    Examples:
        >>> with RegisterTIRExInfo({"/data/test collection/ir_datasets": "trec-2019-train", "foo": "bar"}):
        >>>   pass  # Do tracking here and save to ir-datasets file
    """

    def __init__(self, info: dict[str, VALUE]) -> None:
        self._info = info

    def __enter__(self) -> None:
        register_info(self._info)

    def __exit__(self, exc_type, exc_val, exc_tb) -> None:
        # We need to copy the view on the keys here since deregister_info will modify the underlying dict while keys()
        # is iterated.
        deregister_info(list(self._info.keys()))


def get_info() -> Mapping[str, VALUE]:
    return __info
