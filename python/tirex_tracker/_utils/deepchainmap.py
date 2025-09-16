from __future__ import annotations

from collections import ChainMap
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from typing import _KT, _VT, Mapping


class DeepChainMap(ChainMap["_KT", "_VT"]):
    """Variant of ChainMap that allows direct updates to inner scopes.

    Base on: https://docs.python.org/3/library/collections.html#chainmap-examples-and-recipes
    """

    def push(self, map: Mapping[_KT, _VT]) -> None:
        """Adds the given map as the "topmost" element in the stack of mappings. If keys are requested from the
        ChainMap, the maps are iterated from top to bottom and the value of the first dictionary that contains that key
        is returned.

        Args:
            map (Mapping[_KT, _VT]): The mapping that is added to the top of the ChainMap.
        """
        self.maps.insert(0, map)

    def __setitem__(self, key: _KT, value: _VT) -> None:
        """Adds a new item to the ChainMap by modifying the "topmost" (i.e. last added) dictionary that was added to the
        chain and contains the key.

        Args:
            key (_KT): The key to add the value for.
            value (_VT): The value.
        """
        for mapping in self.maps:
            if key in mapping:
                mapping[key] = value
                return
        self.maps[0][key] = value

    def __delitem__(self, key: _KT) -> None:
        """Deleting an item within the DeepChainMap deletes the last occurrence of it.

        Args:
            key (_KT): The key to delete.

        Examples:
            >>> map = DeepChainMap({"a": "b"}, {"a": "c"})
            >>> print(map["a"])
            c
            >>> del map["a"]
            >>> print(map["a"])
            b
            >>> del map["a"]
            >>> print(map["a"])
            KeyError

        Raises:
            KeyError: If the key was not found.
        """
        for mapping in self.maps:
            if key in mapping:
                del mapping[key]
                # TODO: if mapping is empty now, remove it from self.maps
                return
        raise KeyError(key)
