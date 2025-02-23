from contextlib import contextmanager
from ctypes import (
    cdll,
    CDLL,
    c_char_p,
    c_size_t,
    c_void_p,
    c_int,
    Structure,
    pointer,
    POINTER,
    Array,
    CFUNCTYPE,
)
from functools import wraps
from enum import IntEnum, auto
from pathlib import Path
from typing import (
    NamedTuple,
    Optional,
    Callable,
    cast,
    Collection,
    Iterable,
    Protocol,
    Mapping,
    Generator,
    Any,
    TYPE_CHECKING,
    Generic,
    TypeVar,
    List,
    overload,
    Union,
    MutableMapping,
)
from typing_extensions import ParamSpec  # type: ignore

if TYPE_CHECKING:
    from ctypes import Pointer, _CFunctionType as CFunctionType  # type: ignore
else:
    T = TypeVar("T")

    class Pointer(Generic[T]):
        pass

    CFunctionType = Any


_ENCODING = "ascii"


class Error(IntEnum):
    SUCCESS = 0
    INVALID_ARGUMENT = 1


class Measure(IntEnum):
    # TODO: Specify the enum values explicitly.
    OS_NAME = 0
    OS_KERNEL = auto()
    TIME_ELAPSED_WALL_CLOCK_MS = auto()
    TIME_ELAPSED_USER_MS = auto()
    TIME_ELAPSED_SYSTEM_MS = auto()
    CPU_USED_PROCESS_PERCENT = auto()
    CPU_USED_SYSTEM_PERCENT = auto()
    CPU_AVAILABLE_SYSTEM_CORES = auto()
    CPU_ENERGY_SYSTEM_JOULES = auto()
    CPU_FEATURES = auto()
    CPU_FREQUENCY_MHZ = auto()
    CPU_FREQUENCY_MIN_MHZ = auto()
    CPU_FREQUENCY_MAX_MHZ = auto()
    CPU_VENDOR_ID = auto()
    CPU_BYTE_ORDER = auto()
    CPU_ARCHITECTURE = auto()
    CPU_MODEL_NAME = auto()
    CPU_CORES_PER_SOCKET = auto()
    CPU_THREADS_PER_CORE = auto()
    CPU_CACHES = auto()
    CPU_VIRTUALIZATION = auto()
    RAM_USED_PROCESS_KB = auto()
    RAM_USED_SYSTEM_MB = auto()
    RAM_AVAILABLE_SYSTEM_MB = auto()
    RAM_ENERGY_SYSTEM_JOULES = auto()
    GPU_SUPPORTED = auto()
    GPU_MODEL_NAME = auto()
    GPU_AVAILABLE_SYSTEM_CORES = auto()  # aka. GPU_NUM_CORES
    GPU_USED_PROCESS_PERCENT = auto()
    GPU_USED_SYSTEM_PERCENT = auto()
    GPU_VRAM_USED_PROCESS_MB = auto()
    GPU_VRAM_USED_SYSTEM_MB = auto()
    GPU_VRAM_AVAILABLE_SYSTEM_MB = auto()
    GPU_ENERGY_SYSTEM_JOULES = auto()
    GIT_IS_REPO = auto()
    GIT_HASH = auto()
    GIT_LAST_COMMIT_HASH = auto()
    GIT_BRANCH = auto()
    GIT_BRANCH_UPSTREAM = auto()
    GIT_TAGS = auto()
    GIT_REMOTE_ORIGIN = auto()
    GIT_UNCOMMITTED_CHANGES = auto()
    GIT_UNPUSHED_CHANGES = auto()
    GIT_UNCHECKED_FILES = auto()


_INVALID_MEASURE = -1


ALL_MEASURES = set(Measure)


class Aggregation(IntEnum):
    NO = 1 << 0
    MAX = 1 << 1
    MIN = 1 << 2
    MEAN = 1 << 3


_INVALID_AGGREGATION = -1


ALL_AGGREGATIONS = set(Aggregation)


class TrackingHandle(Structure):
    _fields_ = []


class ResultType(IntEnum):
    STRING = 0
    INTEGER = 1
    FLOATING = 2


class _Result(Structure):
    _fields_ = []


class ResultEntry(NamedTuple):
    source: Measure
    value: str
    type: ResultType


class _ResultEntry(Structure):
    source: int
    value: bytes
    type: int

    _fields_ = [
        ("source", c_int),
        ("value", c_char_p),
        ("type", c_int),
    ]

    def to_result_entry(self) -> ResultEntry:
        return ResultEntry(
            source=Measure(self.source),
            value=self.value.decode(_ENCODING),
            type=ResultType(self.type),
        )


class MeasureConfiguration(NamedTuple):
    measure: Measure
    aggregation: Aggregation


class _MeasureConfiguration(Structure):
    measure: int
    aggregation: int

    _fields_ = [
        ("measure", c_int),
        ("aggregation", c_int),
    ]


_NULL_MEASURE_CONFIGURATION = _MeasureConfiguration(
    _INVALID_MEASURE, _INVALID_AGGREGATION
)


class LogLevel(IntEnum):
    TRACE = 0
    DEBUG = 1
    INFO = 2
    WARN = 3
    ERROR = 4
    CRITICAL = 5


class LogCallback(Protocol):
    def __call__(self, level: LogLevel, component: str, message: str) -> None:
        pass


def _noop_log_callback(level: LogLevel, component: str, message: str):
    return None


def _to_native_log_callback(log_callback: LogCallback) -> CFunctionType:
    @CFUNCTYPE(None, c_int, c_char_p, c_char_p)
    def _log_callback(level: c_int, component: c_char_p, message: c_char_p) -> None:
        if log_callback is _noop_log_callback:
            return  # Do nothing.

        component_bytes = component.value
        if component_bytes is None:
            raise ValueError("Component not set.")
        message_bytes = message.value
        if message_bytes is None:
            raise ValueError("Message not set.")

        log_callback(
            LogLevel(level.value),
            component_bytes.decode(_ENCODING),
            message_bytes.decode(_ENCODING),
        )

    return _log_callback


class ProviderInfo(NamedTuple):
    name: str
    description: str
    version: Optional[str]


class _ProviderInfo(Structure):
    name: bytes
    description: bytes
    version: Optional[bytes]

    _fields_ = [
        ("name", c_char_p),
        ("description", c_char_p),
        ("version", c_char_p),
    ]

    def to_provider(self) -> ProviderInfo:
        return ProviderInfo(
            name=self.name.decode(_ENCODING),
            description=self.description.decode(_ENCODING),
            version=(
                self.version.decode(_ENCODING) if self.version is not None else None
            ),
        )


class MeasureInfo(NamedTuple):
    description: str
    result_type: ResultType
    example: str


class _MeasureInfo(Structure):
    description: bytes
    result_type: int
    example: bytes

    _fields_ = [
        ("description", c_char_p),
        ("result_type", c_int),
        ("example", c_char_p),
    ]

    def to_measure_info(self) -> MeasureInfo:
        return MeasureInfo(
            description=self.description.decode(_ENCODING),
            result_type=ResultType(self.result_type),
            example=self.example.decode(_ENCODING),
        )


class _TirexTrackerLibrary(CDLL):
    msrResultEntryGetByIndex: Callable[
        [Pointer[_Result], int, Pointer[_ResultEntry]], int
    ]
    msrResultEntryNum: Callable[[Pointer[_Result], Pointer[int]], int]
    msrResultFree: Callable[[Pointer[_Result]], None]
    msrFetchInfo: Callable[
        [Array[_MeasureConfiguration], Pointer[Pointer[_Result]]], int
    ]
    msrStartMeasure: Callable[
        [Array[_MeasureConfiguration], int, Pointer[Pointer[TrackingHandle]]], int
    ]
    msrStopMeasure: Callable[
        [Pointer[TrackingHandle], Pointer[Pointer[_Result]]], int
    ]
    # FIXME:
    # msrSetLogCallback: Callable[[CFunctionType], None]
    msrDataProviderGetAll: Callable[[Array[_ProviderInfo], int], int]
    msrMeasureInfoGet: Callable[[int, Pointer[Pointer[_MeasureInfo]]], int]


def _find_library() -> Path:
    return Path(__file__).parent / "libmeasureapi.so"


def _load_library() -> _TirexTrackerLibrary:
    library = cdll.LoadLibrary(str(_find_library()))
    # Note: Not defining the function argument and return types can cause issues with down-casted addresses for the handle in the past.
    library.msrResultEntryGetByIndex.argtypes = [
        POINTER(_Result),
        c_size_t,
        POINTER(_ResultEntry),
    ]
    library.msrResultEntryGetByIndex.restype = c_int
    library.msrResultEntryNum.argtypes = [POINTER(_Result), POINTER(c_size_t)]
    library.msrResultEntryNum.restype = c_int
    library.msrResultFree.argtypes = [POINTER(_Result)]
    library.msrResultFree.restype = c_void_p
    library.msrFetchInfo.argtypes = [
        Array[_MeasureConfiguration],
        POINTER(POINTER(_Result)),
    ]
    library.msrFetchInfo.restype = c_int
    library.msrStartMeasure.argtypes = [
        Array[_MeasureConfiguration],
        c_size_t,
        POINTER(POINTER(TrackingHandle)),
    ]
    library.msrStartMeasure.restype = c_int
    library.msrStopMeasure.argtypes = [
        POINTER(TrackingHandle),
        POINTER(POINTER(_Result)),
    ]
    library.msrStopMeasure.restype = c_int
    # FIXME:
    # library.msrSetLogCallback.argtypes = [c_void_p]
    # library.msrSetLogCallback.restype = c_void_p
    library.msrDataProviderGetAll.argtypes = [Array[_ProviderInfo], c_size_t]
    library.msrDataProviderGetAll.restype = c_size_t
    library.msrMeasureInfoGet.argtypes = [c_int, POINTER(POINTER(_MeasureInfo))]
    library.msrMeasureInfoGet.restype = c_int

    return cast(_TirexTrackerLibrary, library)


_LIBRARY = _load_library()


def _handle_error(error_int: int) -> None:
    error = Error(error_int)
    if error == Error.SUCCESS:
        return
    elif error == Error.INVALID_ARGUMENT:
        raise ValueError("Invalid argument in native call.")


def provider_infos() -> Collection[ProviderInfo]:
    num_providers = _LIBRARY.msrDataProviderGetAll((_ProviderInfo * 0)(), 0)
    if num_providers == 0:
        return []
    providers: Array[_ProviderInfo] = (_ProviderInfo * num_providers)()
    _LIBRARY.msrDataProviderGetAll(providers, num_providers)
    providers_iterable: Iterable[_ProviderInfo] = cast(
        Iterable[_ProviderInfo], providers
    )
    return [provider.to_provider() for provider in providers_iterable]


def measure_infos() -> Mapping[Measure, MeasureInfo]:
    measure_infos: dict[Measure, MeasureInfo] = {}
    for measure in ALL_MEASURES:
        measure_info_pointer = pointer(pointer(_MeasureInfo()))
        error_int = _LIBRARY.msrMeasureInfoGet(measure.value, measure_info_pointer)
        _handle_error(error_int)
        measure_info = measure_info_pointer.contents.contents
        measure_infos[measure] = measure_info.to_measure_info()
    return measure_infos


# TODO: Add aggregation(s) (mapping) parameter.
# TODO: Maybe rename this function.
def fetch_info(
    measures: Iterable[Measure] = ALL_MEASURES,
    log_callback: LogCallback = _noop_log_callback,
) -> Mapping[Measure, ResultEntry]:
    # FIXME
    # _LIBRARY.msrSetLogCallback(_to_native_log_callback(log_callback))

    configs = [
        _MeasureConfiguration(measure.value, Aggregation.NO.value)
        for measure in measures
    ] + [_NULL_MEASURE_CONFIGURATION]
    configs_array = (_MeasureConfiguration * (len(configs)))(*configs)

    result_pointer = pointer(pointer(_Result()))
    error_int = _LIBRARY.msrFetchInfo(configs_array, result_pointer)
    _handle_error(error_int)
    result = result_pointer.contents

    # FIXME
    # _LIBRARY.msrSetLogCallback(_to_native_log_callback(_nop_log_callback))

    num_entries_pointer = pointer(c_size_t())
    error_int = _LIBRARY.msrResultEntryNum(result, num_entries_pointer)
    _handle_error(error_int)
    num_entries = num_entries_pointer.contents.value
    entries: List[ResultEntry] = []
    for index in range(num_entries):
        entry_pointer = pointer(_ResultEntry())
        error_int = _LIBRARY.msrResultEntryGetByIndex(result, index, entry_pointer)
        _handle_error(error_int)
        entries.append(entry_pointer.contents.to_result_entry())
    _LIBRARY.msrResultFree(result)
    return {entry.source: entry for entry in entries}


# TODO: Add aggregation(s) (mapping) parameter.
def start_tracking(
    measures: Iterable[Measure] = ALL_MEASURES,
    poll_intervall_ms: int = -1,
    log_callback: LogCallback = _noop_log_callback,
) -> Pointer[TrackingHandle]:
    # FIXME
    # _LIBRARY.msrSetLogCallback(_to_native_log_callback(log_callback))

    configs = [
        _MeasureConfiguration(measure.value, Aggregation.NO.value)
        for measure in measures
    ] + [_NULL_MEASURE_CONFIGURATION]
    configs_array = (_MeasureConfiguration * (len(configs)))(*configs)

    measurement_handle_pointer = pointer(pointer(TrackingHandle()))
    error_int = _LIBRARY.msrStartMeasure(
        configs_array, poll_intervall_ms, measurement_handle_pointer
    )
    _handle_error(error_int)
    return measurement_handle_pointer.contents


def stop_tracking(
    measurement_handle: Pointer[TrackingHandle],
) -> Mapping[Measure, ResultEntry]:
    result_pointer = pointer(pointer(_Result()))
    error_int = _LIBRARY.msrStopMeasure(measurement_handle, result_pointer)
    _handle_error(error_int)
    result = result_pointer.contents

    # FIXME
    # _LIBRARY.msrSetLogCallback(_to_native_log_callback(_nop_log_callback))

    num_entries_pointer = pointer(c_size_t())
    error_int = _LIBRARY.msrResultEntryNum(result, num_entries_pointer)
    _handle_error(error_int)
    num_entries = num_entries_pointer.contents.value
    entries: List[ResultEntry] = []
    for index in range(num_entries):
        entry_pointer = pointer(_ResultEntry())
        error_int = _LIBRARY.msrResultEntryGetByIndex(result, index, entry_pointer)
        _handle_error(error_int)
        entries.append(entry_pointer.contents.to_result_entry())
    _LIBRARY.msrResultFree(result)
    return {entry.source: entry for entry in entries}


# TODO: Add aggregation(s) (mapping) parameter.
@contextmanager
def tracking(
    measures: Iterable[Measure] = ALL_MEASURES,
    poll_intervall_ms: int = -1,
    log_callback: LogCallback = _noop_log_callback,
) -> Generator[Mapping[Measure, ResultEntry], Any, Mapping[Measure, ResultEntry]]:
    handle = start_tracking(
        measures=measures,
        poll_intervall_ms=poll_intervall_ms,
        log_callback=log_callback,
    )
    results: dict[Measure, ResultEntry] = {}
    try:
        yield results
    finally:
        tmp_results = stop_tracking(handle)
        results.clear()
        results.update(tmp_results)
        return results


P = ParamSpec("P")
T = TypeVar("T")


class ResultsAccessor(Protocol):
    results: Mapping[Measure, ResultEntry]


@overload
def tracked(f_or_measures: Callable[P, T]) -> Union[Callable[P, T], ResultsAccessor]:
    pass


@overload
def tracked(
    f_or_measures: Iterable[Measure] = ALL_MEASURES,
    poll_intervall_ms: int = ...,
    log_callback: LogCallback = ...,
) -> Callable[[Callable[P, T]], Union[Callable[P, T], ResultsAccessor]]:
    pass


# TODO: Add aggregation(s) (mapping) parameter.
def tracked(
    f_or_measures: Union[Callable[P, T], Iterable[Measure]] = ALL_MEASURES,
    poll_intervall_ms: int = -1,
    log_callback: LogCallback = _noop_log_callback,
) -> Union[
    Union[Callable[P, T], ResultsAccessor],
    Callable[[Callable[P, T]], Union[Callable[P, T], ResultsAccessor]],
]:
    if callable(f_or_measures):
        f = f_or_measures

        results: MutableMapping[Measure, ResultEntry] = {}

        @wraps(f)
        def wrapper(*args, **kwds):
            nonlocal results
            handle = start_tracking()
            try:
                return f(*args, **kwds)
            finally:
                results.clear()
                tmp_results = stop_tracking(handle)
                results.update(tmp_results)

        results_wrapper = cast(Union[Callable[P, T], ResultsAccessor], wrapper)
        results_wrapper.results = results  # type: ignore
        return results_wrapper

    else:
        measures = f_or_measures

        def decorator(f: Callable[P, T]) -> Union[Callable[P, T], ResultsAccessor]:
            results: MutableMapping[Measure, ResultEntry] = {}

            @wraps(f)
            def wrapper(*args, **kwds):
                nonlocal results
                handle = start_tracking(
                    measures=measures,
                    poll_intervall_ms=poll_intervall_ms,
                    log_callback=log_callback,
                )
                try:
                    return f(*args, **kwds)
                finally:
                    results.clear()
                    tmp_results = stop_tracking(handle)
                    results.update(tmp_results)

            results_wrapper = cast(Union[Callable[P, T], ResultsAccessor], wrapper)
            results_wrapper.results = results  # type: ignore
            return results_wrapper

        return decorator
