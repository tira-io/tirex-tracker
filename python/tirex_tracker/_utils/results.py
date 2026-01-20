from __future__ import annotations

from ctypes import c_char_p, c_wchar_p, cast
from typing import TYPE_CHECKING

from tirex_tracker import Measure, ResultEntry, ResultType
from tirex_tracker import _ResultEntry as NativeResultEntry

if TYPE_CHECKING:
    from typing import Any, Callable, Dict


_native_result_entry_parsers: Dict[ResultType, Callable[[int], Any]] = {
    ResultType.STRING: lambda ptr: cast(ptr, c_char_p).value,
    ResultType.INTEGER: lambda ptr: NotImplemented,  # TODO: implement
    ResultType.FLOATING: lambda ptr: NotImplemented,  # TODO: implement
    ResultType.BOOLEAN: lambda ptr: NotImplemented,  # TODO: implement
    ResultType.WSTRING: lambda ptr: cast(ptr, c_wchar_p).value,
    ResultType.STRING_LIST: lambda ptr: NotImplemented,  # TODO: implement
    ResultType.INTEGER_LIST: lambda ptr: NotImplemented,  # TODO: implement
    ResultType.FLOATING_LIST: lambda ptr: NotImplemented,  # TODO: implement
    ResultType.BOOLEAN_LIST: lambda ptr: NotImplemented,  # TODO: implement
    ResultType.WSTRING_LIST: lambda ptr: NotImplemented,  # TODO: implement
}


def parse_native_result_entry(entry: NativeResultEntry) -> ResultEntry:
    """
    Translates a native result entry into a Python managed object. The Python object can safely be accessed even after
    the result entry is freed.

    :param entry: The native result entry object that should be translated into a Python managed result entry object.
    :type entry: NativeResultEntry
    :return: A Python managed result entry object. The native object can now safely be deleted.
    :rtype: ResultEntry
    """
    rtype = ResultType(entry.type)
    value = _native_result_entry_parsers[rtype](entry.value)
    return ResultEntry(source=Measure(entry.source), value=value, type=rtype)
