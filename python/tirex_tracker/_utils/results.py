from __future__ import annotations

from ctypes import c_char_p, c_wchar_p, cast
from typing import TYPE_CHECKING

from .. import Measure, ResultEntry, ResultType
from .. import _ResultEntry as NativeResultEntry

if TYPE_CHECKING:
    from typing import Any, Callable, Dict


_to_python_type: Dict[ResultType, Callable[[int], Any]] = {
    ResultType.STRING: lambda ptr: cast(ptr, c_char_p).value,
    # ResultType.INTEGER: TODO,
    # ResultType.FLOATING: TODO,
    # ResultType.BOOLEAN: TODO,
    ResultType.WSTRING: lambda ptr: cast(ptr, c_wchar_p).value,
    # ResultType.STRING_LIST: TODO,
    # ResultType.INTEGER_LIST: TODO,
    # ResultType.FLOATING_LIST: TODO,
    # ResultType.BOOLEAN_LIST: TODO,
    # ResultType.WSTRING_LIST: TODO,
}


def parse_native_result_entry(entry: NativeResultEntry) -> ResultEntry:
    rtype = ResultType(entry.type)
    value = _to_python_type[rtype](entry.value)
    return ResultEntry(source=Measure(entry.source), value=value, type=rtype)
