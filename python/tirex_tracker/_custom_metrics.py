from __future__ import annotations

# We need collections.abc.Mapping here instead of typing.Mapping since we make use of polymorphism
from collections.abc import Mapping
from typing import TYPE_CHECKING

from box import Box

from ._utils.deepchainmap import DeepChainMap

if TYPE_CHECKING:
    from typing import Iterable, MutableMapping, Union

    VALUE = Union[Mapping[str, "VALUE"], str]

__info: DeepChainMap[str, VALUE] = DeepChainMap()


def register_info(info: dict[Union[str, tuple[str, ...]], VALUE]) -> None:
    """Registers the provided information. If the key is a tuple, it is considered to be a "path". The ir_metadata
    export will use the exact path to place the metadata into the export.

    Unless absoluteley necessary, it is recommended to use `RegisterTIRExInfo` instead of `register_info` and
    `deregister_info`.

    Examples:
        >>> registerInfo({"hello": "world", "foo": "bar"})
        >>> deregisterInfo(["hello", "foo"])
    """
    ndict: Box = Box()
    for keys, v in info.items():
        if isinstance(keys, tuple):
            cur = ndict
            for k in keys[:-1]:
                if k not in cur or not isinstance(cur[k], dict):
                    cur[k] = Box()
                cur = cur[k]
            cur[keys[-1]] = v
        else:
            assert isinstance(keys, str)
            ndict[keys] = v

    __info.push(ndict)


def deregister_info(keys: Iterable[Union[str, tuple[str, ...]]]) -> None:
    """Removes the last registered occurence of each key. Every info that is registered must equally often be
    deregistered. If a key is registered twice (first for value `v1`, then for value `v2`) then it will hold the value
    of its last registration (`v2`). Deregistration "pops" the last value such that it holds the value of the
    penultimate registration (`v1`). After another deregistration, the value is undefined for the key (KeyError).

    Unless absoluteley necessary, it is recommended to use `RegisterTIRExInfo` instead of `register_info` and
    `deregister_info`.

    Args:
        keys (Iterable[str]): The keys that should be deregistered.
    """

    def _rec_remove(dic: MutableMapping, key: Union[str, tuple[str, ...]]) -> None:
        if isinstance(key, tuple) and len(key) == 1:
            key = key[0]
        if isinstance(key, str):
            del dic[key]
        else:
            _rec_remove(dic[key[0]], key[1:])
            # Clean up by removing empty dictionaries
            if len(dic[key[0]]) == 0:
                del dic[key[0]]

    for key in keys:
        _rec_remove(__info, key)


class RegisterTIRExInfo:
    """Registers and deregisters the provided metrics.

    Examples:
        >>> with RegisterTIRExInfo({("data", "test collection", "ir_datasets"): "trec-2019-train", "foo": "bar"}):
        >>>   pass  # Do tracking here and save to ir-datasets file
    """

    def __init__(self, info: dict[Union[str, tuple[str, ...]], VALUE]) -> None:
        self._info = info

    def __enter__(self) -> None:
        register_info(self._info)

    def __exit__(self, exc_type, exc_val, exc_tb) -> None:
        # We need to copy the view on the keys here since deregister_info will modify the underlying dict while keys()
        # is iterated.
        deregister_info(list(self._info.keys()))


def get_info() -> Mapping[str, VALUE]:
    """Retrieves the current user-provided information as it was provided using `RegisterTIRExInfo` or
    (less recommended) `register_info` and `deregister_info`.

    Returns:
        the current user-provided information.
    """
    return __info.to_dict()
