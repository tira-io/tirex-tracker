from __future__ import annotations

from collections import defaultdict
from ctypes import CFUNCTYPE, c_char_p, c_int, c_size_t, pointer
from dataclasses import dataclass
from enum import Enum
from functools import wraps
from gzip import open as gzip_open
from io import BytesIO
from json import dumps, loads
from pathlib import Path
from shutil import copy
from sys import argv, executable, version_info
from sys import modules as sys_modules
from traceback import extract_stack
from typing import (
    IO,
    TYPE_CHECKING,
    Any,
    Callable,
    Collection,
    ContextManager,
    ItemsView,
    Iterable,
    Iterator,
    KeysView,
    List,
    Mapping,
    MutableMapping,
    Optional,
    Protocol,
    Tuple,
    TypeVar,
    Union,
    ValuesView,
    cast,
    overload,
)

from importlib_metadata import distributions
from IPython.core.getipython import get_ipython
from typing_extensions import ParamSpec, Self, TypeAlias  # type: ignore
from yaml import safe_dump as yaml_safe_dump
from yaml import safe_load as yaml_safe_load

from ._utils.constants import ALL_AGGREGATIONS, ALL_MEASURES, Aggregation, Error, LogLevel, Measure, ResultType
from ._utils.constants import ENCODING as _ENCODING
from ._utils.errorhandling import ABORT_HANDLE as _ABORT_HANDLE
from ._utils.errorhandling import deinit_error_handling as _deinit_error_handling
from ._utils.errorhandling import init_error_handling as _init_error_handling
from ._utils.library import (
    _PYTHON_MEASURES,
    MeasureInfo,
    ProviderInfo,
    ResultEntry,
    _MeasureConfiguration,
    _MeasureInfo,
    _ProviderInfo,
    _Result,
    _ResultEntry,
    _TrackingHandle,
    provider_version,
)
from ._utils.library import LIBRARY as _LIBRARY
from ._utils.library import (
    NULL_MEASURE_CONFIGURATION as _NULL_MEASURE_CONFIGURATION,
)
from .archive_utils import create_code_archive, git_repo_or_none

if TYPE_CHECKING:
    from ctypes import Array
    from ctypes import _CFunctionType as CFunctionType  # type: ignore
    from ctypes import _Pointer as Pointer  # type: ignore

PathLike: TypeAlias = Optional[Union[str, Path]]

P = ParamSpec("P")
T = TypeVar("T")

_REGISTERED_METADATA: MutableMapping[str, Any] = {}
_REGISTERED_FILES: List[Tuple[Path, Path, str]] = []


class ResultsAccessor(Protocol):
    results: Mapping[Measure, ResultEntry]


class LogCallback(Protocol):
    def __call__(self, level: LogLevel, component: str, message: str) -> None:
        pass


_PYTHON_PROVIDER = ProviderInfo(
    name="Python",
    description="Python-specific measures.",
    version=provider_version,
)


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
    modules = sorted({module.split(".")[0] for module in sys_modules if not module.startswith("_")})
    _add_python_result_entry(
        results=results,
        measure=Measure.PYTHON_MODULES,
        measures=measures,
        value=modules,
    )
    installed_packages = sorted({f"{distribution.name}=={distribution.version}" for distribution in distributions()})
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

    measures = {measure for measure in measures if measure not in _PYTHON_MEASURES}

    return results, measures


def _deep_merge(dict1: dict, dict2: dict) -> dict:
    for key in dict2:
        if key in dict1 and isinstance(dict1[key], dict) and isinstance(dict2[key], dict):
            _deep_merge(dict1[key], dict2[key])
        else:
            dict1[key] = dict2[key]
    return dict1


def _recursive_defaultdict() -> dict:
    return defaultdict(_recursive_defaultdict)


def _recursive_undefaultdict(dict: dict) -> dict:
    return {
        key: (_recursive_undefaultdict(value) if isinstance(value, defaultdict) else value)
        for key, value in dict.items()
    }


class ExportFormat(Enum):
    IR_METADATA = "ir_metadata"


def _handle_error(error_int: int) -> None:
    error = Error(error_int)
    if error == Error.SUCCESS:
        return
    elif error == Error.INVALID_ARGUMENT:
        raise ValueError("Invalid argument in native call.")
    raise RuntimeError(f"An error occured: {error_int}")


__callback: "Optional[CFunctionType]"


def set_log_callback(log_callback: Optional[LogCallback] = None) -> None:
    def _to_native_log_callback(log_callback: LogCallback):
        @CFUNCTYPE(None, c_int, c_char_p, c_char_p)
        def _log_callback(level: int, component: bytes, message: bytes) -> None:
            log_callback(LogLevel(level), component.decode(_ENCODING), message.decode(_ENCODING))

        return _log_callback

    global __callback  # We need to store a reference to the callback to avoid deletion through GC
    __callback = _to_native_log_callback(log_callback) if log_callback is not None else None

    _LIBRARY.tirexSetLogCallback(__callback)


def provider_infos() -> Collection[ProviderInfo]:
    num_providers = _LIBRARY.tirexDataProviderGetAll((_ProviderInfo * 0)(), 0)
    if num_providers == 0:
        return []
    providers: "Array[_ProviderInfo]" = (_ProviderInfo * num_providers)()
    _LIBRARY.tirexDataProviderGetAll(providers, num_providers)
    providers_iterable: Iterable[_ProviderInfo] = cast(Iterable[_ProviderInfo], providers)
    return [provider.to_provider() for provider in providers_iterable] + [_PYTHON_PROVIDER]


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
    from tirex_tracker._utils.results import parse_native_result_entry  # avoid circular imports

    num_entries_pointer = pointer(c_size_t())
    error_int = _LIBRARY.tirexResultEntryNum(result, num_entries_pointer)
    _handle_error(error_int)
    num_entries = num_entries_pointer.contents.value
    entries: List[ResultEntry] = []
    for index in range(num_entries):
        entry_pointer = pointer(_ResultEntry())
        error_int = _LIBRARY.tirexResultEntryGetByIndex(result, c_size_t(index), entry_pointer)
        _handle_error(error_int)
        entries.append(parse_native_result_entry(entry_pointer.contents))
    _LIBRARY.tirexResultFree(result)
    results = {entry.source: entry for entry in entries}
    return results


def _prepare_measure_configurations(
    measures: Iterable[Measure],
) -> "Array[_MeasureConfiguration]":
    configs = [_MeasureConfiguration(measure.value, Aggregation.NO.value) for measure in measures] + [
        _NULL_MEASURE_CONFIGURATION
    ]
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
    _error_handle: _ABORT_HANDLE
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
        python_info, measures = _get_python_info(measures=measures, export_file_path=export_file_path)

        # Prepare the measure configurations.
        configs_array = _prepare_measure_configurations(measures)

        # Get other info, first, before starting the tracking.
        result_pointer = pointer(pointer(_Result()))
        error_int = _LIBRARY.tirexFetchInfo(configs_array, result_pointer)
        _handle_error(error_int)
        fetch_info_result = result_pointer.contents

        # Instate the error handler
        error_handle = _init_error_handling()

        # Start the tracking.
        tracking_handle_pointer = pointer(pointer(_TrackingHandle()))
        error_int = _LIBRARY.tirexStartTracking(configs_array, poll_intervall_ms, tracking_handle_pointer)
        _handle_error(error_int)
        tracking_handle = tracking_handle_pointer.contents

        return cls(
            _fetch_info_result=fetch_info_result,
            _tracking_handle=tracking_handle,
            _error_handle=error_handle,
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

        _deinit_error_handling(self._error_handle)

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

    def get(self, key: Measure, default: Optional[T] = None) -> Optional[Union[ResultEntry, T]]:  # type: ignore[overload]
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
                ".yml",
                ".yaml",
                "ir_metadata.gz",
                "ir-metadata.gz",
                "irmetadata.gz",
                ".yml.gz",
                ".yaml.gz",
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
        ir_metadata["implementation"]["executable"]["cmd"] = loads(self._python_info[Measure.PYTHON_EXECUTABLE].value)
        ir_metadata["implementation"]["executable"]["args"] = loads(self._python_info[Measure.PYTHON_ARGUMENTS].value)
        ir_metadata["implementation"]["executable"]["version"] = loads(self._python_info[Measure.PYTHON_VERSION].value)
        ir_metadata["implementation"]["python"]["modules"] = loads(self._python_info[Measure.PYTHON_VERSION].value)
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
                self._python_info[Measure.PYTHON_NOTEBOOK_FILE_PATH_IN_CODE_ARCHIVE].value
            )

        ir_metadata = _deep_merge(ir_metadata, tmp_ir_metadata)
        ir_metadata = _recursive_undefaultdict(ir_metadata)

        global _REGISTERED_METADATA
        if _REGISTERED_METADATA:
            ir_metadata.update(_REGISTERED_METADATA)

        for registered_resolve_to, registered_file, subdir in _REGISTERED_FILES:
            if subdir is None:
                target_file = export_file_path.parent / registered_file
            else:
                target_file = export_file_path.parent / subdir / registered_file
            target_file.parent.mkdir(exist_ok=True, parents=True)
            copy(registered_resolve_to / registered_file, target_file)

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


def clear_metadata_register() -> None:
    global _REGISTERED_METADATA
    _REGISTERED_METADATA = {}


def register_metadata(metadata: Mapping[str, Any]) -> None:
    global _REGISTERED_METADATA
    _REGISTERED_METADATA.update(metadata)


def clear_file_register() -> None:
    global _REGISTERED_FILES
    _REGISTERED_FILES = []


def register_file(resolve_to: Path, file: Path, subdirectory: str = ".") -> None:
    if not resolve_to.is_dir():
        raise ValueError(f"Tirex-tracker resolve_to should point to an directory, got {resolve_to}")
    if not (resolve_to / file).is_file():
        raise ValueError(f"Tirex-tracker resolve_to/file should exist, got {resolve_to / file}")

    _REGISTERED_FILES.append((resolve_to, file, subdirectory))


## Initialize error handling to abort at level CRITICAL
_LIBRARY.tirexSetAbortLevel(LogLevel.CRITICAL)

__all__ = ["ALL_AGGREGATIONS", "ALL_MEASURES", "Aggregation", "Error", "LogLevel", "Measure", "ResultType", "tracking"]
