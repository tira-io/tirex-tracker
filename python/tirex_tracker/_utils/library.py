import sys
from ctypes import CDLL, POINTER, Structure, c_char_p, c_int, c_size_t, c_void_p, cdll
from importlib import resources
from json import dumps
from pathlib import Path
from sys import platform
from typing import TYPE_CHECKING, Callable, ContextManager, Mapping, NamedTuple, Optional, cast

from importlib_metadata import PackageNotFoundError, version

if TYPE_CHECKING:
    from ctypes import Array
    from ctypes import _CFunctionType as CFunctionType  # type: ignore
    from ctypes import _Pointer as Pointer  # type: ignore

from .constants import ENCODING, INVALID_AGGREGATION, INVALID_MEASURE, Aggregation, Measure, ResultType


class _TrackingHandle(Structure):
    _fields_ = []


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


NULL_MEASURE_CONFIGURATION = _MeasureConfiguration(INVALID_MEASURE, INVALID_AGGREGATION)


class _Result(Structure):
    _fields_ = []


class ResultEntry(NamedTuple):
    source: Measure
    value: str
    type: ResultType


class _ResultEntry(Structure):
    source: int
    value: int
    type: int

    _fields_ = [
        ("source", c_int),
        ("value", c_void_p),
        ("type", c_int),
    ]


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
            description=self.description.decode(ENCODING),
            data_type=ResultType(self.result_type),
            example=self.example.decode(ENCODING),
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
            name=self.name.decode(ENCODING),
            description=self.description.decode(ENCODING),
            version=(self.version.decode(ENCODING) if self.version is not None else None),
        )


try:
    provider_version = version("tirex-tracker")
except PackageNotFoundError:
    # if the TIREx tracker is not installed via pip
    provider_version = "unpublished"


class _TirexTrackerLibrary(CDLL):
    tirexResultEntryGetByIndex: Callable[["Pointer[_Result]", c_size_t, "Pointer[_ResultEntry]"], int]
    tirexResultEntryNum: Callable[["Pointer[_Result]", "Pointer[c_size_t]"], int]
    tirexResultFree: Callable[["Pointer[_Result]"], None]
    tirexFetchInfo: Callable[["Array[_MeasureConfiguration]", "Pointer[Pointer[_Result]]"], int]
    tirexStartTracking: Callable[["Array[_MeasureConfiguration]", int, "Pointer[Pointer[_TrackingHandle]]"], int]
    tirexStopTracking: Callable[["Pointer[_TrackingHandle]", "Pointer[Pointer[_Result]]"], int]
    tirexSetLogCallback: Callable[["Optional[CFunctionType]"], None]
    tirexDataProviderGetAll: Callable[["Array[_ProviderInfo]", int], int]
    tirexMeasureInfoGet: Callable[[int, "Pointer[Pointer[_MeasureInfo]]"], int]
    tirexResultExportIrMetadata: Callable[["Pointer[_Result]", "Pointer[_Result]", c_char_p], int]


def __get_library() -> ContextManager[Path]:
    path: str
    if platform == "linux":
        path = "libtirex_tracker.so"
    elif platform == "darwin":
        path = "libtirex_tracker.dylib"
    elif platform == "win32":
        path = "tirex_tracker.dll"
    else:
        raise RuntimeError("Unsupported platform.")
    top_package = __package__.split(".", maxsplit=1)[0]  # Should evaluate to "tirex_tracker"
    if sys.version_info >= (3, 9):  # Python 3.9 or later:
        return resources.as_file(resources.files(top_package).joinpath(path))
    # Python 3.7, Python 3.8: (this is deprecated and should not be used in later versions)
    return resources.path(top_package, path)


def _load_library() -> _TirexTrackerLibrary:
    with __get_library() as lib:
        library = cdll.LoadLibrary(str(lib))
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
    library.tirexSetAbortLevel.argtypes = [c_int]
    library.tirexSetAbortCallback.argtypes = [c_void_p]
    return cast(_TirexTrackerLibrary, library)


LIBRARY = _load_library()
