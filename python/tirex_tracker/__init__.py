from collections import defaultdict
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
    CFUNCTYPE,
)
from dataclasses import dataclass
from enum import IntEnum, Enum
from functools import wraps
from gzip import open as gzip_open
from importlib_metadata import distributions, version
from importlib_resources import files
from io import BytesIO
from json import dumps, loads
from pathlib import Path
from sys import modules as sys_modules, executable, argv, platform, version_info
from traceback import extract_stack
from typing import (
    IO,
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
    TypeVar,
    List,
    overload,
    Union,
    MutableMapping,
    Tuple,
    ContextManager,
)

from IPython import get_ipython
from typing_extensions import ParamSpec, Self, TypeAlias  # type: ignore
from yaml import safe_load as yaml_safe_load, safe_dump as yaml_safe_dump

from tirex_tracker.archive_utils import create_code_archive, git_repo_or_none


if TYPE_CHECKING:
    from ctypes import _Pointer as Pointer, _CFunctionType as CFunctionType, Array  # type: ignore

PathLike: TypeAlias = Optional[Union[str, Path]]

P = ParamSpec("P")
T = TypeVar("T")


_ENCODING = "ascii"


class Error(IntEnum):
    SUCCESS = 0
    INVALID_ARGUMENT = 1


class Measure(IntEnum):
    OS_NAME = 0
    OS_KERNEL = 1
    TIME_START = 44
    TIME_STOP = 45
    TIME_ELAPSED_WALL_CLOCK_MS = 2
    TIME_ELAPSED_USER_MS = 3
    TIME_ELAPSED_SYSTEM_MS = 4
    CPU_USED_PROCESS_PERCENT = 5
    CPU_USED_SYSTEM_PERCENT = 6
    CPU_AVAILABLE_SYSTEM_CORES = 7
    CPU_ENERGY_SYSTEM_JOULES = 8
    CPU_FEATURES = 9
    CPU_FREQUENCY_MHZ = 10
    CPU_FREQUENCY_MIN_MHZ = 11
    CPU_FREQUENCY_MAX_MHZ = 12
    CPU_VENDOR_ID = 13
    CPU_BYTE_ORDER = 14
    CPU_ARCHITECTURE = 15
    CPU_MODEL_NAME = 16
    CPU_CORES_PER_SOCKET = 17
    CPU_THREADS_PER_CORE = 18
    CPU_CACHES = 19
    CPU_VIRTUALIZATION = 20
    RAM_USED_PROCESS_KB = 21
    RAM_USED_SYSTEM_MB = 22
    RAM_AVAILABLE_SYSTEM_MB = 23
    RAM_ENERGY_SYSTEM_JOULES = 24
    GPU_SUPPORTED = 25
    GPU_MODEL_NAME = 26
    GPU_AVAILABLE_SYSTEM_CORES = 27  # aka. GPU_NUM_CORES
    GPU_USED_PROCESS_PERCENT = 28
    GPU_USED_SYSTEM_PERCENT = 29
    GPU_VRAM_USED_PROCESS_MB = 30
    GPU_VRAM_USED_SYSTEM_MB = 31
    GPU_VRAM_AVAILABLE_SYSTEM_MB = 32
    GPU_ENERGY_SYSTEM_JOULES = 33
    GIT_IS_REPO = 34
    GIT_HASH = 35
    GIT_LAST_COMMIT_HASH = 36
    GIT_BRANCH = 37
    GIT_BRANCH_UPSTREAM = 38
    GIT_TAGS = 39
    GIT_REMOTE_ORIGIN = 40
    GIT_UNCOMMITTED_CHANGES = 41
    GIT_UNPUSHED_CHANGES = 42
    GIT_UNCHECKED_FILES = 43
    PYTHON_VERSION = 1000
    PYTHON_EXECUTABLE = 1001
    PYTHON_ARGUMENTS = 1002
    PYTHON_MODULES = 1003
    PYTHON_INSTALLED_PACKAGES = 1004
    PYTHON_IS_INTERACTIVE = 1005
    PYTHON_SCRIPT_FILE_PATH = 1006
    # 1007 was used in previous versions of the library.
    PYTHON_NOTEBOOK_FILE_PATH = 1008
    # 1009 was used in previous versions of the library.
    PYTHON_CODE_ARCHIVE_PATH = 1010
    PYTHON_SCRIPT_FILE_PATH_IN_CODE_ARCHIVE = 1011
    PYTHON_NOTEBOOK_FILE_PATH_IN_CODE_ARCHIVE = 1012


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


def _to_native_log_callback(log_callback: LogCallback) -> "CFunctionType":
    @CFUNCTYPE(c_void_p, c_int, c_char_p, c_char_p)
    def _log_callback(level: c_int, component: c_char_p, message: c_char_p) -> c_void_p:
        if log_callback is _noop_log_callback:
            return c_void_p()  # Do nothing.

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

        return c_void_p()

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


_PYTHON_PROVIDER = ProviderInfo(
    name="Python",
    description="Python-specific measures.",
    version=version("tirex-tracker"),
)


class MeasureInfo(NamedTuple):
    description: str
    data_type: ResultType
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
            data_type=ResultType(self.result_type),
            example=self.example.decode(_ENCODING),
        )


_PYTHON_MEASURES: Mapping[Measure, MeasureInfo] = {
    Measure.PYTHON_VERSION: MeasureInfo(
        description="Python version used to run the tracked program.",
        data_type=ResultType.STRING,
        example=dumps("3.12.0"),
    ),
    Measure.PYTHON_EXECUTABLE: MeasureInfo(
        description="Python executable used to run the program.",
        data_type=ResultType.STRING,
        example=dumps("/usr/bin/python3"),
    ),
    Measure.PYTHON_ARGUMENTS: MeasureInfo(
        description="Arguments passed to the Python executable.",
        data_type=ResultType.STRING_LIST,
        example=dumps(["-m", "tirex_tracker"]),
    ),
    Measure.PYTHON_MODULES: MeasureInfo(
        description="Python modules visible in the current environment.",
        data_type=ResultType.STRING_LIST,
        example=dumps(["os", "sys", "json"]),
    ),
    Measure.PYTHON_INSTALLED_PACKAGES: MeasureInfo(
        description="Python packages installed in the current environment.",
        data_type=ResultType.STRING_LIST,
        example=dumps(["tirex-tracker==0.1.0", "numpy==1.21.2"]),
    ),
    Measure.PYTHON_IS_INTERACTIVE: MeasureInfo(
        description="True if the Python interpreter is run interactively.",
        data_type=ResultType.BOOLEAN,
        example=dumps(True),
    ),
    Measure.PYTHON_SCRIPT_FILE_PATH: MeasureInfo(
        description="Path to the Python script file.",
        data_type=ResultType.STRING,
        example=dumps("/path/to/script.py"),
    ),
    Measure.PYTHON_NOTEBOOK_FILE_PATH: MeasureInfo(
        description="Path to the Jupyter notebook file.",
        data_type=ResultType.STRING,
        example=dumps("/path/to/notebook.ipynb"),
    ),
    Measure.PYTHON_CODE_ARCHIVE_PATH: MeasureInfo(
        description="The archive that contains a snapshot of the code.",
        data_type=ResultType.STRING,
        example=dumps("/path/to/code.zip"),
    ),
    Measure.PYTHON_SCRIPT_FILE_PATH_IN_CODE_ARCHIVE: MeasureInfo(
        description="The script that was executed in the code archive.",
        data_type=ResultType.STRING,
        example=dumps("script.py"),
    ),
    Measure.PYTHON_NOTEBOOK_FILE_PATH_IN_CODE_ARCHIVE: MeasureInfo(
        description="The notebook that was executed in the code archive.",
        data_type=ResultType.STRING,
        example=dumps("notebook.ipynb"),
    ),
}


def _python_result_entry(measure: Measure, value: Any) -> ResultEntry:
    return ResultEntry(
        source=measure,
        value=dumps(value),
        type=_PYTHON_MEASURES[measure].data_type,
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


def _get_python_info(
    measures: Iterable[Measure], export_file_path: Optional[PathLike] = None
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

    if ipython is None:
        script_file_path = Path(extract_stack()[0].filename).resolve()
        repo = git_repo_or_none(script_file_path)
        if repo is not None and repo.working_tree_dir is not None:
            script_file_path = script_file_path.relative_to(repo.working_tree_dir)
        _add_python_result_entry(
            results=results,
            measure=Measure.PYTHON_SCRIPT_FILE_PATH,
            measures=measures,
            value=str(script_file_path),
        )

    if export_file_path is not None:
        # Create a utility directory as a sibling to the export file, containing the code archive.
        metadata_directory_path = Path(export_file_path).parent / ".tirex-tracker"

        # Clear and create the directory.
        if metadata_directory_path.exists():
            metadata_directory_path.rmdir()
        metadata_directory_path.mkdir(parents=True)

        archive_paths = create_code_archive(metadata_directory_path)

        _add_python_result_entry(
            results=results,
            measure=Measure.PYTHON_CODE_ARCHIVE_PATH,
            measures=measures,
            value=str(archive_paths.zip_file_path),
        )
        _add_python_result_entry(
            results=results,
            measure=Measure.PYTHON_SCRIPT_FILE_PATH_IN_CODE_ARCHIVE,
            measures=measures,
            value=str(archive_paths.script_file_path_in_zip),
        )
        if archive_paths.notebook_file_path_in_zip is not None:
            _add_python_result_entry(
                results=results,
                measure=Measure.PYTHON_NOTEBOOK_FILE_PATH_IN_CODE_ARCHIVE,
                measures=measures,
                value=str(archive_paths.notebook_file_path_in_zip),
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


def _recursive_undefaultdict(dict: dict) -> dict:
    return {
        key: _recursive_undefaultdict(value)
        if isinstance(value, defaultdict)
        else value
        for key, value in dict.items()
    }


class ExportFormat(Enum):
    IR_METADATA = "ir_metadata"


class _TirexTrackerLibrary(CDLL):
    tirexResultEntryGetByIndex: Callable[
        ["Pointer[_Result]", c_size_t, "Pointer[_ResultEntry]"], int
    ]
    tirexResultEntryNum: Callable[["Pointer[_Result]", "Pointer[c_size_t]"], int]
    tirexResultFree: Callable[["Pointer[_Result]"], None]
    tirexFetchInfo: Callable[
        ["Array[_MeasureConfiguration]", "Pointer[Pointer[_Result]]"], int
    ]
    tirexStartTracking: Callable[
        ["Array[_MeasureConfiguration]", int, "Pointer[Pointer[_TrackingHandle]]"], int
    ]
    tirexStopTracking: Callable[
        ["Pointer[_TrackingHandle]", "Pointer[Pointer[_Result]]"], int
    ]
    tirexSetLogCallback: Callable[["CFunctionType"], None]
    tirexDataProviderGetAll: Callable[["Array[_ProviderInfo]", int], int]
    tirexMeasureInfoGet: Callable[[int, "Pointer[Pointer[_MeasureInfo]]"], int]
    tirexResultExportIrMetadata: Callable[
        ["Pointer[_Result]", "Pointer[_Result]", c_char_p], int
    ]


def _find_library() -> Path:
    path: str
    if platform == "linux":
        path = "libtirex_tracker.so"
    elif platform == "darwin":
        path = "libtirex_tracker.dylib"
    elif platform == "win32":
        path = "tirex_tracker.dll"
    else:
        raise RuntimeError("Unsupported platform.")
    return files(__name__) / path


def _load_library() -> _TirexTrackerLibrary:
    library = cdll.LoadLibrary(str(_find_library()))
    library.tirexResultEntryGetByIndex.argtypes = [
        POINTER(_Result),
        c_size_t,
        POINTER(_ResultEntry),
    ]
    library.tirexResultEntryGetByIndex.restype = c_int
    library.tirexResultEntryNum.argtypes = [POINTER(_Result), POINTER(c_size_t)]
    library.tirexResultEntryNum.restype = c_int
    library.tirexResultFree.argtypes = [POINTER(_Result)]
    library.tirexResultFree.restype = c_void_p
    library.tirexFetchInfo.argtypes = [
        POINTER(_MeasureConfiguration),
        POINTER(POINTER(_Result)),
    ]
    library.tirexFetchInfo.restype = c_int
    library.tirexStartTracking.argtypes = [
        POINTER(_MeasureConfiguration),
        c_size_t,
        POINTER(POINTER(_TrackingHandle)),
    ]
    library.tirexStartTracking.restype = c_int
    library.tirexStopTracking.argtypes = [
        POINTER(_TrackingHandle),
        POINTER(POINTER(_Result)),
    ]
    library.tirexStopTracking.restype = c_int
    library.tirexSetLogCallback.argtypes = [c_void_p]
    library.tirexSetLogCallback.restype = c_void_p
    library.tirexDataProviderGetAll.argtypes = [POINTER(_ProviderInfo), c_size_t]
    library.tirexDataProviderGetAll.restype = c_size_t
    library.tirexMeasureInfoGet.argtypes = [c_int, POINTER(POINTER(_MeasureInfo))]
    library.tirexMeasureInfoGet.restype = c_int
    library.tirexResultExportIrMetadata.argtypes = [
        POINTER(_Result),
        POINTER(_Result),
        c_char_p,
    ]
    library.tirexResultExportIrMetadata.restype = c_int
    return cast(_TirexTrackerLibrary, library)


_LIBRARY = _load_library()


def _handle_error(error_int: int) -> None:
    error = Error(error_int)
    if error == Error.SUCCESS:
        return
    elif error == Error.INVALID_ARGUMENT:
        raise ValueError("Invalid argument in native call.")


def set_log_callback(log_callback: LogCallback = _noop_log_callback) -> None:
    _LIBRARY.tirexSetLogCallback(_to_native_log_callback(log_callback))


def provider_infos() -> Collection[ProviderInfo]:
    num_providers = _LIBRARY.tirexDataProviderGetAll((_ProviderInfo * 0)(), 0)
    if num_providers == 0:
        return []
    providers: "Array[_ProviderInfo]" = (_ProviderInfo * num_providers)()
    _LIBRARY.tirexDataProviderGetAll(providers, num_providers)
    providers_iterable: Iterable[_ProviderInfo] = cast(
        Iterable[_ProviderInfo], providers
    )
    return [provider.to_provider() for provider in providers_iterable] + [
        _PYTHON_PROVIDER
    ]


def measure_infos() -> Mapping[Measure, MeasureInfo]:
    measure_infos: MutableMapping[Measure, MeasureInfo] = {}
    for measure in ALL_MEASURES:
        if measure in _PYTHON_MEASURES:
            measure_infos[measure] = _PYTHON_MEASURES[measure]
            continue
        measure_info_pointer = pointer(pointer(_MeasureInfo()))
        error_int = _LIBRARY.tirexMeasureInfoGet(measure.value, measure_info_pointer)
        _handle_error(error_int)
        measure_info = measure_info_pointer.contents.contents
        measure_infos[measure] = measure_info.to_measure_info()
    return measure_infos


def _parse_results(result: "Pointer[_Result]") -> Mapping[Measure, ResultEntry]:
    num_entries_pointer = pointer(c_size_t())
    error_int = _LIBRARY.tirexResultEntryNum(result, num_entries_pointer)
    _handle_error(error_int)
    num_entries = num_entries_pointer.contents.value
    entries: List[ResultEntry] = []
    for index in range(num_entries):
        entry_pointer = pointer(_ResultEntry())
        error_int = _LIBRARY.tirexResultEntryGetByIndex(
            result, c_size_t(index), entry_pointer
        )
        _handle_error(error_int)
        entries.append(entry_pointer.contents.to_result_entry())
    _LIBRARY.tirexResultFree(result)
    results = {entry.source: entry for entry in entries}
    return results


def _prepare_measure_configurations(
    measures: Iterable[Measure],
) -> "Array[_MeasureConfiguration]":
    configs = [
        _MeasureConfiguration(measure.value, Aggregation.NO.value)
        for measure in measures
    ] + [_NULL_MEASURE_CONFIGURATION]
    configs_array = (_MeasureConfiguration * (len(configs)))(*configs)
    return configs_array


# TODO: Add aggregation(s) (mapping) parameter.
def fetch_info(
    measures: Iterable[Measure] = ALL_MEASURES,
) -> Mapping[Measure, ResultEntry]:
    # Get Python info first, and then strip Python measures from the list.
    python_info, remaining_measures = _get_python_info(measures)

    # Prepare the measure configurations.
    configs_array = _prepare_measure_configurations(remaining_measures)

    result_pointer = pointer(pointer(_Result()))
    error_int = _LIBRARY.tirexFetchInfo(configs_array, result_pointer)
    _handle_error(error_int)

    return {
        **_parse_results(result_pointer.contents),
        **python_info,
    }


@dataclass(frozen=True)
class TrackingHandle(ContextManager["TrackingHandle"], Mapping[Measure, ResultEntry]):
    _fetch_info_result: "Pointer[_Result]"
    _tracking_handle: "Pointer[_TrackingHandle]"
    _python_info: Mapping[Measure, ResultEntry]
    _system_name: Optional[str]
    _system_description: Optional[str]
    _export_file_path: Optional[PathLike]
    _export_format: Optional[ExportFormat]
    results: MutableMapping[Measure, ResultEntry]

    # TODO: Add aggregation(s) (mapping) parameter.
    @classmethod
    def start(
        cls,
        measures: Iterable[Measure] = ALL_MEASURES,
        poll_intervall_ms: int = 100,
        system_name: Optional[str] = None,
        system_description: Optional[str] = None,
        export_file_path: Optional[PathLike] = None,
        export_format: Optional[ExportFormat] = None,
    ) -> Self:
        # Get Python info first, and then strip Python measures from the list.
        python_info, measures = _get_python_info(
            measures=measures, export_file_path=export_file_path
        )

        # Prepare the measure configurations.
        configs_array = _prepare_measure_configurations(measures)

        # Get other info, first, before starting the tracking.
        result_pointer = pointer(pointer(_Result()))
        error_int = _LIBRARY.tirexFetchInfo(configs_array, result_pointer)
        _handle_error(error_int)
        fetch_info_result = result_pointer.contents

        # Start the tracking.
        tracking_handle_pointer = pointer(pointer(_TrackingHandle()))
        error_int = _LIBRARY.tirexStartTracking(
            configs_array, poll_intervall_ms, tracking_handle_pointer
        )
        _handle_error(error_int)
        tracking_handle = tracking_handle_pointer.contents

        return cls(
            _fetch_info_result=fetch_info_result,
            _tracking_handle=tracking_handle,
            _python_info=python_info,
            _system_name=system_name,
            _system_description=system_description,
            _export_file_path=export_file_path,
            _export_format=export_format,
            results={},
        )

    def stop(self) -> Mapping[Measure, ResultEntry]:
        result_pointer = pointer(pointer(_Result()))
        error_int = _LIBRARY.tirexStopTracking(self._tracking_handle, result_pointer)
        _handle_error(error_int)

        self._export(result_pointer.contents)

        self.results.update(_parse_results(self._fetch_info_result))
        self.results.update(self._python_info)
        self.results.update(_parse_results(result_pointer.contents))
        return self.results

    def __exit__(self, exc_type, exc_value, traceback) -> None:
        self.stop()

    def __len__(self) -> int:
        return len(self.results)

    def __iter__(self) -> Iterator[Measure]:
        return iter(self.results)

    def __getitem__(self, key) -> ResultEntry:
        return self.results[key]

    @overload
    def get(self, key: Measure) -> Optional[ResultEntry]: ...

    @overload
    def get(self, key: Measure, default: T) -> Union[ResultEntry, T]: ...
    def get(
        self, key: Measure, default: Optional[T] = None
    ) -> Optional[Union[ResultEntry, T]]:
        return self.results.get(key, default)

    def __contains__(self, key) -> bool:
        return key in self.results

    def keys(self) -> KeysView[Measure]:
        return self.results.keys()

    def items(self) -> ItemsView[Measure, ResultEntry]:
        return self.results.items()

    def values(self) -> ValuesView[ResultEntry]:
        return self.results.values()

    def __eq__(self, other) -> bool:
        return NotImplemented

    def _export(self, result: "Pointer[_Result]") -> None:
        if self._export_file_path is None:
            return
        elif self._export_format is None:
            self._export_guessed_format(result)
        elif self._export_format == ExportFormat.IR_METADATA:
            self._export_ir_metadata(result)
        else:
            raise ValueError("Invalid export format.")

    def _export_guessed_format(self, result: "Pointer[_Result]") -> None:
        if self._export_file_path is None:
            return
        elif any(
            Path(self._export_file_path).name.endswith(extension)
            for extension in [
                "ir_metadata",
                "ir-metadata",
                "irmetadata",
                "ir_metadata.yml",
                "ir-metadata.yml",
                "irmetadata.yml",
                "ir_metadata.yaml",
                "ir-metadata.yaml",
                "irmetadata.yaml",
                "ir_metadata.gz",
                "ir-metadata.gz",
                "irmetadata.gz",
                "ir_metadata.yml.gz",
                "ir-metadata.yml.gz",
                "irmetadata.yml.gz",
                "ir_metadata.yaml.gz",
                "ir-metadata.yaml.gz",
                "irmetadata.yaml.gz",
            ]
        ):
            self._export_ir_metadata(result)

    def _export_ir_metadata(self, result: "Pointer[_Result]") -> None:
        if self._export_file_path is None:
            return

        export_file_path = Path(self._export_file_path)
        if export_file_path.exists():
            raise ValueError("Metadata file already exists.")

        # Run the C-internal ir_metadata export.
        _LIBRARY.tirexResultExportIrMetadata(
            self._fetch_info_result,
            result,
            c_char_p(str(export_file_path.resolve()).encode(_ENCODING)),
        )

        # Parse the initial ir_metadata.
        buffer = Path(export_file_path).read_bytes()
        if buffer.startswith(b"ir_metadata.start\n"):
            buffer = buffer[len(b"ir_metadata.start\n") :]
        if buffer.endswith(b"ir_metadata.end\n"):
            buffer = buffer[: -len(b"ir_metadata.end\n")]

        with BytesIO(buffer) as yaml_file:
            tmp_ir_metadata = yaml_safe_load(yaml_file)
        ir_metadata = _recursive_defaultdict()

        # Add user-provided metadata.
        if self._system_name is not None:
            ir_metadata["method"]["name"] = self._system_name
        if self._system_description is not None:
            ir_metadata["method"]["description"] = self._system_description

        # Add Python-specific metadata.
        ir_metadata["implementation"]["executable"]["cmd"] = loads(
            self._python_info[Measure.PYTHON_EXECUTABLE].value
        )
        ir_metadata["implementation"]["executable"]["args"] = loads(
            self._python_info[Measure.PYTHON_ARGUMENTS].value
        )
        ir_metadata["implementation"]["executable"]["version"] = loads(
            self._python_info[Measure.PYTHON_VERSION].value
        )
        ir_metadata["implementation"]["python"]["modules"] = loads(
            self._python_info[Measure.PYTHON_VERSION].value
        )
        ir_metadata["implementation"]["python"]["packages"] = loads(
            self._python_info[Measure.PYTHON_INSTALLED_PACKAGES].value
        )
        ir_metadata["implementation"]["python"]["interactive"] = loads(
            self._python_info[Measure.PYTHON_IS_INTERACTIVE].value
        )
        if Measure.PYTHON_SCRIPT_FILE_PATH in self._python_info:
            ir_metadata["implementation"]["script"]["path"] = loads(
                self._python_info[Measure.PYTHON_SCRIPT_FILE_PATH].value
            )
        if Measure.PYTHON_NOTEBOOK_FILE_PATH in self._python_info:
            ir_metadata["implementation"]["notebook"]["path"] = loads(
                self._python_info[Measure.PYTHON_NOTEBOOK_FILE_PATH].value
            )
        ir_metadata["implementation"]["source"]["archive"]["path"] = loads(
            self._python_info[Measure.PYTHON_CODE_ARCHIVE_PATH].value
        )
        ir_metadata["implementation"]["source"]["archive"]["script path"] = loads(
            self._python_info[Measure.PYTHON_SCRIPT_FILE_PATH_IN_CODE_ARCHIVE].value
        )
        if Measure.PYTHON_NOTEBOOK_FILE_PATH_IN_CODE_ARCHIVE in self._python_info:
            ir_metadata["implementation"]["source"]["archive"]["notebook path"] = loads(
                self._python_info[
                    Measure.PYTHON_NOTEBOOK_FILE_PATH_IN_CODE_ARCHIVE
                ].value
            )

        ir_metadata = _deep_merge(ir_metadata, tmp_ir_metadata)
        ir_metadata = _recursive_undefaultdict(ir_metadata)

        # Serialize the updated ir_metadata.
        file_open: Callable[[], IO[str]]
        if export_file_path.suffix == ".gz":

            def file_open() -> IO[str]:
                return gzip_open(export_file_path, "wt")
        else:

            def file_open() -> IO[str]:
                return export_file_path.open("wt")

        write_prefix_suffix = any(
            Path(self._export_file_path).name.endswith(extension)
            for extension in [
                "ir_metadata",
                "ir-metadata",
                "irmetadata",
                "ir_metadata.gz",
                "ir-metadata.gz",
                "irmetadata.gz",
            ]
        )
        with file_open() as file:
            if write_prefix_suffix:
                file.write("ir_metadata.start\n")

            yaml_safe_dump(
                data=ir_metadata,
                stream=file,
                encoding="utf-8",
            )
            if write_prefix_suffix:
                file.write("ir_metadata.end\n")


# TODO: Add aggregation(s) (mapping) parameter.
def start_tracking(
    measures: Iterable[Measure] = ALL_MEASURES,
    poll_intervall_ms: int = 100,
    system_name: Optional[str] = None,
    system_description: Optional[str] = None,
    export_file_path: Optional[PathLike] = None,
    export_format: Optional[ExportFormat] = None,
) -> TrackingHandle:
    return TrackingHandle.start(
        measures=measures,
        poll_intervall_ms=poll_intervall_ms,
        system_name=system_name,
        system_description=system_description,
        export_file_path=export_file_path,
        export_format=export_format,
    )


def stop_tracking(
    tracking_handle: TrackingHandle,
) -> Mapping[Measure, ResultEntry]:
    return tracking_handle.stop()


# TODO: Add aggregation(s) (mapping) parameter.
def tracking(
    measures: Iterable[Measure] = ALL_MEASURES,
    poll_intervall_ms: int = 100,
    system_name: Optional[str] = None,
    system_description: Optional[str] = None,
    export_file_path: Optional[PathLike] = None,
    export_format: Optional[ExportFormat] = None,
) -> TrackingHandle:
    return TrackingHandle.start(
        measures=measures,
        poll_intervall_ms=poll_intervall_ms,
        system_name=system_name,
        system_description=system_description,
        export_file_path=None if export_file_path is None else Path(export_file_path),
        export_format=export_format,
    )


# TODO: Add aggregation(s) (mapping) parameter.
def track(
    block: Callable[[], None],
    measures: Iterable[Measure] = ALL_MEASURES,
    poll_intervall_ms: int = 100,
    system_name: Optional[str] = None,
    system_description: Optional[str] = None,
    export_file_path: Optional[PathLike] = None,
    export_format: Optional[ExportFormat] = None,
) -> Mapping[Measure, ResultEntry]:
    with tracking(
        measures=measures,
        poll_intervall_ms=poll_intervall_ms,
        system_name=system_name,
        system_description=system_description,
        export_file_path=export_file_path,
        export_format=export_format,
    ) as handle:
        block()
        return handle.results


@overload
def tracked(f_or_measures: Callable[P, T]) -> Union[Callable[P, T], ResultsAccessor]:
    pass


@overload
def tracked(
    f_or_measures: Iterable[Measure] = ALL_MEASURES,
    poll_intervall_ms: int = ...,
    system_name: Optional[str] = ...,
    system_description: Optional[str] = ...,
    export_file_path: Optional[PathLike] = ...,
    export_format: Optional[ExportFormat] = ...,
) -> Callable[[Callable[P, T]], Union[Callable[P, T], ResultsAccessor]]:
    pass


# TODO: Add aggregation(s) (mapping) parameter.
def tracked(
    f_or_measures: Union[Callable[P, T], Iterable[Measure]] = ALL_MEASURES,
    poll_intervall_ms: int = 100,
    system_name: Optional[str] = None,
    system_description: Optional[str] = None,
    export_file_path: Optional[PathLike] = None,
    export_format: Optional[ExportFormat] = None,
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
            handle = TrackingHandle.start()
            try:
                return f(*args, **kwds)
            finally:
                tmp_results = handle.stop()
                results.clear()
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
                handle = TrackingHandle.start(
                    measures=measures,
                    poll_intervall_ms=poll_intervall_ms,
                    system_name=system_name,
                    system_description=system_description,
                    export_file_path=export_file_path,
                    export_format=export_format,
                )
                try:
                    return f(*args, **kwds)
                finally:
                    tmp_results = handle.stop()
                    results.clear()
                    results.update(tmp_results)

            results_wrapper = cast(Union[Callable[P, T], ResultsAccessor], wrapper)
            results_wrapper.results = results  # type: ignore
            return results_wrapper

        return decorator
