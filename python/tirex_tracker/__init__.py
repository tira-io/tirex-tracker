from collections import defaultdict
from contextlib import redirect_stdout, AbstractContextManager
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
from importlib.metadata import distributions, version
from io import BytesIO
from json import dumps, loads
from os import PathLike
from pathlib import Path
from sys import modules as sys_modules, executable, argv, version_info
from tempfile import TemporaryDirectory
from traceback import extract_stack
from typing import (
    ItemsView,
    Iterator,
    KeysView,
    NamedTuple,
    Optional,
    Callable,
    ValuesView,
    cast,
    Collection,
    Iterable,
    Protocol,
    Mapping,
    Any,
    TYPE_CHECKING,
    Generic,
    TypeVar,
    List,
    overload,
    Union,
    MutableMapping,
    Tuple,
)

from IPython import get_ipython
from typing_extensions import ParamSpec  # type: ignore
from yaml import safe_load, dump


if TYPE_CHECKING:
    from ctypes import _Pointer as Pointer, _CFunctionType as CFunctionType  # type: ignore
else:
    T = TypeVar("T")

    class Pointer(Generic[T]):
        pass

    CFunctionType = Any

P = ParamSpec("P")
T = TypeVar("T")


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
    PYTHON_VERSION = 1000
    PYTHON_EXECUTABLE = 1001
    PYTHON_ARGUMENTS = 1002
    PYTHON_MODULES = 1003
    PYTHON_INSTALLED_PACKAGES = 1004
    PYTHON_IS_INTERACTIVE = 1005
    PYTHON_SCRIPT_FILE_PATH = 1006
    PYTHON_SCRIPT_FILE_CONTENTS = 1007
    PYTHON_NOTEBOOK_FILE_PATH = 1008
    PYTHON_NOTEBOOK_FILE_CONTENTS = 1009


_INVALID_MEASURE = -1


ALL_MEASURES = set(Measure)


class Aggregation(IntEnum):
    NO = 1 << 0
    MAX = 1 << 1
    MIN = 1 << 2
    MEAN = 1 << 3


_INVALID_AGGREGATION = -1


ALL_AGGREGATIONS = set(Aggregation)


class _TrackingHandle(Structure):
    _fields_ = []


class ResultType(IntEnum):
    STRING = 0
    INTEGER = 1
    FLOATING = 2
    STRING_LIST = 3
    BOOLEAN = 4


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


class ResultsAccessor(Protocol):
    results: Mapping[Measure, ResultEntry]


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


_PYTHON_PROVIDER = ProviderInfo(
    name="Python",
    description="Python-specific measures.",
    version=version("tirex-tracker"),
)


_PYTHON_MEASURES: Mapping[Measure, MeasureInfo] = {
    Measure.PYTHON_VERSION: MeasureInfo(
        description="Python version used to run the tracked program.",
        result_type=ResultType.STRING,
        example=dumps("3.12.0"),
    ),
    Measure.PYTHON_EXECUTABLE: MeasureInfo(
        description="Python executable used to run the program.",
        result_type=ResultType.STRING,
        example=dumps("/usr/bin/python3"),
    ),
    Measure.PYTHON_ARGUMENTS: MeasureInfo(
        description="Arguments passed to the Python executable.",
        result_type=ResultType.STRING_LIST,
        example=dumps(["-m", "tirex_tracker"]),
    ),
    Measure.PYTHON_MODULES: MeasureInfo(
        description="Python modules visible in the current environment.",
        result_type=ResultType.STRING_LIST,
        example=dumps(["os", "sys", "json"]),
    ),
    Measure.PYTHON_INSTALLED_PACKAGES: MeasureInfo(
        description="Python packages installed in the current environment.",
        result_type=ResultType.STRING_LIST,
        example=dumps(["tirex-tracker==0.1.0", "numpy==1.21.2"]),
    ),
    Measure.PYTHON_IS_INTERACTIVE: MeasureInfo(
        description="True if the Python interpreter is run interactively.",
        result_type=ResultType.BOOLEAN,
        example=dumps(True),
    ),
    Measure.PYTHON_SCRIPT_FILE_PATH: MeasureInfo(
        description="Path to the Python script file.",
        result_type=ResultType.STRING,
        example=dumps("/path/to/script.py"),
    ),
    Measure.PYTHON_SCRIPT_FILE_CONTENTS: MeasureInfo(
        description="Contents of the Python script file.",
        result_type=ResultType.STRING,
        example=dumps("""
print("Hello, World!")
"""),
    ),
    Measure.PYTHON_NOTEBOOK_FILE_PATH: MeasureInfo(
        description="Path to the Jupyter notebook file.",
        result_type=ResultType.STRING,
        example=dumps("/path/to/notebook.ipynb"),
    ),
    Measure.PYTHON_NOTEBOOK_FILE_CONTENTS: MeasureInfo(
        description="Contents of the Jupyter notebook file.",
        result_type=ResultType.STRING,
        example=dumps("""
{
 "cells": [
  {
   "cell_type": "code",
   "execution_count": null,
   "metadata": {},
   "outputs": [],
   "source": [
    "print(\"Hello, World!\")"
   ]
  }
 ],
 "metadata": {
  "language_info": {
   "name": "python"
  }
 },
 "nbformat": 4,
 "nbformat_minor": 2
}
"""),
    ),
}


def _python_result_entry(measure: Measure, value: Any) -> ResultEntry:
    return ResultEntry(
        source=measure,
        value=dumps(value),
        type=_PYTHON_MEASURES[measure].result_type,
    )


def _add_python_result_entry(
    results: MutableMapping[Measure, ResultEntry],
    measure: Measure,
    measures: Iterable[Measure],
    value: Any,
) -> None:
    if measure not in measures:
        return
    results[measure] = _python_result_entry(measure, value)


def get_python_info(
    measures: Iterable[Measure],
) -> Tuple[Mapping[Measure, ResultEntry], Iterable[Measure]]:
    results: MutableMapping[Measure, ResultEntry] = {}

    _add_python_result_entry(
        results=results,
        measure=Measure.PYTHON_VERSION,
        measures=measures,
        value=f"{version_info.major}.{version_info.minor}.{version_info.micro}",
    )
    _add_python_result_entry(
        results=results,
        measure=Measure.PYTHON_EXECUTABLE,
        measures=measures,
        value=executable,
    )
    _add_python_result_entry(
        results=results,
        measure=Measure.PYTHON_ARGUMENTS,
        measures=measures,
        value=argv,
    )
    modules = sorted(
        {
            module.split(".")[0]
            for module in sys_modules.keys()
            if not module.startswith("_")
        }
    )
    _add_python_result_entry(
        results=results,
        measure=Measure.PYTHON_MODULES,
        measures=measures,
        value=modules,
    )
    installed_packages = sorted(
        {
            f"{distribution.name}=={distribution.version}"
            for distribution in distributions()
        }
    )
    _add_python_result_entry(
        results=results,
        measure=Measure.PYTHON_INSTALLED_PACKAGES,
        measures=measures,
        value=installed_packages,
    )

    ipython = get_ipython()
    is_interactive = ipython is not None
    _add_python_result_entry(
        results=results,
        measure=Measure.PYTHON_IS_INTERACTIVE,
        measures=measures,
        value=is_interactive,
    )

    if ipython is not None:
        # TODO: Add IPython-specific metadata to `ir_metadata`.
        with TemporaryDirectory(delete=False) as temp_dir:
            tmp_dir_path = Path(temp_dir)

            script_file_path = tmp_dir_path / "script.py"
            _add_python_result_entry(
                results=results,
                measure=Measure.PYTHON_SCRIPT_FILE_PATH,
                measures=measures,
                value=script_file_path,
            )

            with redirect_stdout(None):
                ipython.magic(f"save -f {script_file_path} 1-9999")

            with script_file_path.open("rt") as file:
                script_file_contents = file.read()
            _add_python_result_entry(
                results=results,
                measure=Measure.PYTHON_SCRIPT_FILE_CONTENTS,
                measures=measures,
                value=script_file_contents,
            )

            notebook_file_path = tmp_dir_path / "notebook.ipynb"
            _add_python_result_entry(
                results=results,
                measure=Measure.PYTHON_NOTEBOOK_FILE_PATH,
                measures=measures,
                value=notebook_file_path,
            )

            with redirect_stdout(None):
                ipython.magic(f"notebook {notebook_file_path}")
            with notebook_file_path.open("rt") as file:
                notebook_file_contents = file.read()
            _add_python_result_entry(
                results=results,
                measure=Measure.PYTHON_NOTEBOOK_FILE_CONTENTS,
                measures=measures,
                value=notebook_file_contents,
            )

    else:
        script_file_path = Path(extract_stack()[0].filename).resolve()
        _add_python_result_entry(
            results=results,
            measure=Measure.PYTHON_SCRIPT_FILE_PATH,
            measures=measures,
            value=script_file_path,
        )

        with script_file_path.open("rt") as file:
            script_file_contents = file.read()
        _add_python_result_entry(
            results=results,
            measure=Measure.PYTHON_SCRIPT_FILE_CONTENTS,
            measures=measures,
            value=script_file_contents,
        )

        _add_python_result_entry(
            results=results,
            measure=Measure.PYTHON_NOTEBOOK_FILE_PATH,
            measures=measures,
            value=None,
        )
        _add_python_result_entry(
            results=results,
            measure=Measure.PYTHON_NOTEBOOK_FILE_CONTENTS,
            measures=measures,
            value=None,
        )

    measures = {
        measure for measure in measures if measure not in _PYTHON_MEASURES.keys()
    }

    return results, measures


def _deep_merge(dict1: dict, dict2: dict) -> dict:
    for key in dict2:
        if (
            key in dict1
            and isinstance(dict1[key], dict)
            and isinstance(dict2[key], dict)
        ):
            _deep_merge(dict1[key], dict2[key])
        else:
            dict1[key] = dict2[key]
    return dict1


def _recursive_defaultdict() -> dict:
    return defaultdict(_recursive_defaultdict)


def _export_ir_metadata(
    tracking_handle: Pointer[_TrackingHandle],
    python_info: Mapping[Measure, ResultEntry],
    output_directory: PathLike = Path.cwd(),
    system_name: Optional[str] = None,
    system_description: Optional[str] = None,
    system_is_indexing: bool = False,
) -> None:
    output_directory_path = Path(output_directory)
    if output_directory_path.exists() and not output_directory_path.is_dir():
        raise ValueError("The output directory must be a directory.")
    if not output_directory_path.exists():
        output_directory_path.mkdir(parents=True, exist_ok=True)
    output_file_path = output_directory_path / ".ir-metadata"
    if output_file_path.exists():
        raise ValueError("Metadata file already exists in the output directory.")

    # TODO: Run the C-internal ir_metadata export from the `tracking_handle` to `output_file_path`.
    tracking_handle

    # Parse the initial ir_metadata.
    with output_file_path.open("rb") as file:
        buffer = file.read()
        buffer.removeprefix(b"ir_metadata.start\n")
        buffer.removesuffix(b"ir_metadata.end\n")
        with BytesIO(buffer) as yaml_file:
            tmp_ir_metadata = safe_load(yaml_file)

    ir_metadata = _recursive_defaultdict()
    ir_metadata = _deep_merge(ir_metadata, tmp_ir_metadata)

    # Add user-provided metadata.
    if system_name is not None or system_description is not None:
        method: dict = {}
        if system_name is not None:
            method["name"] = system_name
        if system_description is not None:
            method["description"] = system_description

        if system_is_indexing:
            ir_metadata["method"]["indexing"] = method
        else:
            if "retrieval" not in ir_metadata["method"]:
                ir_metadata["method"]["retrieval"] = [method]
            else:
                ir_metadata["method"]["retrieval"] += [method]
    if system_name is not None:
        # TODO: Where should the system_name be stored in the ir_metadata?
        pass

    if system_description is not None:
        # TODO: Where should the system_description be stored in the ir_metadata?
        pass

    # Add Python-specific metadata.
    ir_metadata["implementation"]["executable"]["cmd"] = loads(
        python_info[Measure.PYTHON_EXECUTABLE].value
    )
    ir_metadata["implementation"]["executable"]["args"] = loads(
        python_info[Measure.PYTHON_ARGUMENTS].value
    )
    ir_metadata["implementation"]["executable"]["version"] = loads(
        python_info[Measure.PYTHON_VERSION].value
    )
    ir_metadata["implementation"]["python"]["modules"] = loads(
        python_info[Measure.PYTHON_VERSION].value
    )
    ir_metadata["implementation"]["python"]["packages"] = loads(
        python_info[Measure.PYTHON_INSTALLED_PACKAGES].value
    )
    ir_metadata["implementation"]["python"]["interactive"] = loads(
        python_info[Measure.PYTHON_IS_INTERACTIVE].value
    )
    ir_metadata["implementation"]["script"]["path"] = loads(
        python_info[Measure.PYTHON_SCRIPT_FILE_PATH].value
    )
    ir_metadata["implementation"]["script"]["contents"] = loads(
        python_info[Measure.PYTHON_SCRIPT_FILE_CONTENTS].value
    )
    ir_metadata["implementation"]["notebook"]["path"] = loads(
        python_info[Measure.PYTHON_NOTEBOOK_FILE_PATH].value
    )
    ir_metadata["implementation"]["notebook"]["contents"] = loads(
        python_info[Measure.PYTHON_NOTEBOOK_FILE_CONTENTS].value
    )

    # Serialize the updated ir_metadata.
    with output_file_path.open("wb") as file:
        file.write(b"ir_metadata.start\n")
        dump(ir_metadata, file)
        file.write(b"ir_metadata.end\n")


class _TirexTrackerLibrary(CDLL):
    msrResultEntryGetByIndex: Callable[
        [Pointer[_Result], c_size_t, Pointer[_ResultEntry]], int
    ]
    msrResultEntryNum: Callable[[Pointer[_Result], Pointer[c_size_t]], int]
    msrResultFree: Callable[[Pointer[_Result]], None]
    msrFetchInfo: Callable[
        [Array[_MeasureConfiguration], Pointer[Pointer[_Result]]], int
    ]
    msrStartMeasure: Callable[
        [Array[_MeasureConfiguration], int, Pointer[Pointer[_TrackingHandle]]], int
    ]
    msrStopMeasure: Callable[[Pointer[_TrackingHandle], Pointer[Pointer[_Result]]], int]
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
        POINTER(POINTER(_TrackingHandle)),
    ]
    library.msrStartMeasure.restype = c_int
    library.msrStopMeasure.argtypes = [
        POINTER(_TrackingHandle),
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


def set_log_callback(log_callback: LogCallback = _noop_log_callback) -> None:
    # FIXME
    # _LIBRARY.msrSetLogCallback(_to_native_log_callback(log_callback))
    return


def provider_infos() -> Collection[ProviderInfo]:
    num_providers = _LIBRARY.msrDataProviderGetAll((_ProviderInfo * 0)(), 0)
    if num_providers == 0:
        return []
    providers: Array[_ProviderInfo] = (_ProviderInfo * num_providers)()
    _LIBRARY.msrDataProviderGetAll(providers, num_providers)
    providers_iterable: Iterable[_ProviderInfo] = cast(
        Iterable[_ProviderInfo], providers
    )
    return [provider.to_provider() for provider in providers_iterable] + [
        _PYTHON_PROVIDER
    ]


def measure_infos() -> Mapping[Measure, MeasureInfo]:
    measure_infos: dict[Measure, MeasureInfo] = {}
    for measure in ALL_MEASURES:
        if measure in _PYTHON_MEASURES:
            measure_infos[measure] = _PYTHON_MEASURES[measure]
            continue
        measure_info_pointer = pointer(pointer(_MeasureInfo()))
        error_int = _LIBRARY.msrMeasureInfoGet(measure.value, measure_info_pointer)
        _handle_error(error_int)
        measure_info = measure_info_pointer.contents.contents
        measure_infos[measure] = measure_info.to_measure_info()
    return measure_infos


def _parse_results(result: Pointer[_Result]) -> Mapping[Measure, ResultEntry]:
    num_entries_pointer = pointer(c_size_t())
    error_int = _LIBRARY.msrResultEntryNum(result, num_entries_pointer)
    _handle_error(error_int)
    num_entries = num_entries_pointer.contents.value
    entries: List[ResultEntry] = []
    for index in range(num_entries):
        entry_pointer = pointer(_ResultEntry())
        error_int = _LIBRARY.msrResultEntryGetByIndex(
            result, c_size_t(index), entry_pointer
        )
        _handle_error(error_int)
        entries.append(entry_pointer.contents.to_result_entry())
    _LIBRARY.msrResultFree(result)
    results = {entry.source: entry for entry in entries}
    return results


def _prepare_measure_configurations(
    measures: Iterable[Measure],
) -> Array[_MeasureConfiguration]:
    configs = [
        _MeasureConfiguration(measure.value, Aggregation.NO.value)
        for measure in measures
    ] + [_NULL_MEASURE_CONFIGURATION]
    configs_array = (_MeasureConfiguration * (len(configs)))(*configs)
    return configs_array


# TODO: Add aggregation(s) (mapping) parameter.
# TODO: Maybe rename this function.
def fetch_info(
    measures: Iterable[Measure] = ALL_MEASURES,
) -> Mapping[Measure, ResultEntry]:
    # Get Python info first, and then strip Python measures from the list.
    python_info, measures = get_python_info(measures=measures)

    # Prepare the measure configurations.
    configs_array = _prepare_measure_configurations(measures)

    result_pointer = pointer(pointer(_Result()))
    error_int = _LIBRARY.msrFetchInfo(configs_array, result_pointer)
    _handle_error(error_int)

    return {
        **_parse_results(result_pointer.contents),
        **python_info,
    }


class TrackingHandle(
    AbstractContextManager["TrackingHandle", None], Mapping[Measure, ResultEntry]
):
    _handle: Pointer[_TrackingHandle]
    _python_info: Mapping[Measure, ResultEntry]
    _results: MutableMapping[Measure, ResultEntry] = {}

    def __init__(
        self,
        measures: Iterable[Measure] = ALL_MEASURES,
        poll_intervall_ms: int = -1,
    ) -> None:
        # Get Python info first, and then strip Python measures from the list.
        python_info, measures = get_python_info(measures=measures)

        # Prepare the measure configurations.
        configs_array = _prepare_measure_configurations(measures)

        tracking_handle_pointer = pointer(pointer(_TrackingHandle()))
        error_int = _LIBRARY.msrStartMeasure(
            configs_array, poll_intervall_ms, tracking_handle_pointer
        )
        _handle_error(error_int)

        self._handle = tracking_handle_pointer.contents
        self._python_info = python_info

    def stop(self) -> Mapping[Measure, ResultEntry]:
        result_pointer = pointer(pointer(_Result()))
        error_int = _LIBRARY.msrStopMeasure(self._handle, result_pointer)
        _handle_error(error_int)

        self._results.clear()
        self._results.update(
            {
                **_parse_results(result_pointer.contents),
                **self._python_info,
            }
        )
        return self._results

    def __exit__(self, exc_type, exc_value, traceback) -> None:
        self.stop()

    @property
    def results(self) -> Mapping[Measure, ResultEntry]:
        return self._results

    def __len__(self) -> int:
        return len(self._results)

    def __iter__(self) -> Iterator[Measure]:
        return iter(self._results)

    def __getitem__(self, key) -> ResultEntry:
        return self._results[key]

    @overload
    def get(self, key: Measure) -> Optional[ResultEntry]: ...

    @overload
    def get(self, key: Measure, default: T) -> Union[ResultEntry, T]: ...
    def get(
        self, key: Measure, default: Optional[T] = None
    ) -> Optional[Union[ResultEntry, T]]:
        return self._results.get(key, default)

    def __contains__(self, key) -> bool:
        return key in self._results

    def keys(self) -> KeysView[Measure]:
        return self._results.keys()

    def items(self) -> ItemsView[Measure, ResultEntry]:
        return self._results.items()

    def values(self) -> ValuesView[ResultEntry]:
        return self._results.values()

    def __eq__(self, other) -> bool:
        return NotImplemented


# TODO: Add aggregation(s) (mapping) parameter.
def start_tracking(
    measures: Iterable[Measure] = ALL_MEASURES,
    poll_intervall_ms: int = -1,
) -> TrackingHandle:
    return TrackingHandle(
        measures=measures,
        poll_intervall_ms=poll_intervall_ms,
    )


def stop_tracking(
    tracking_handle: TrackingHandle,
) -> Mapping[Measure, ResultEntry]:
    return tracking_handle.stop()


# TODO: Add aggregation(s) (mapping) parameter.
def tracking(
    measures: Iterable[Measure] = ALL_MEASURES,
    poll_intervall_ms: int = -1,
) -> TrackingHandle:
    return start_tracking(
        measures=measures,
        poll_intervall_ms=poll_intervall_ms,
    )


@overload
def tracked(f_or_measures: Callable[P, T]) -> Union[Callable[P, T], ResultsAccessor]:
    pass


@overload
def tracked(
    f_or_measures: Iterable[Measure] = ALL_MEASURES,
    poll_intervall_ms: int = ...,
) -> Callable[[Callable[P, T]], Union[Callable[P, T], ResultsAccessor]]:
    pass


# TODO: Add aggregation(s) (mapping) parameter.
def tracked(
    f_or_measures: Union[Callable[P, T], Iterable[Measure]] = ALL_MEASURES,
    poll_intervall_ms: int = -1,
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
