import ctypes

import os
import urllib.request
from pathlib import Path

LIB_URL = "https://github.com/tira-io/measure/releases/download/v0.0.5/libmeasureapi.so"  # TODO: Instead, bundle the library with the package.

def path_to_libmeasureapi() -> Path:
    target_file = Path(__file__).parent / 'libmeasureapi.so'

    if not os.path.exists(target_file):
        urllib.request.urlretrieve(LIB_URL, target_file)

    return target_file


class MapiConfig(ctypes.Structure):
    _fields_ = [("provider", ctypes.POINTER(ctypes.c_char_p)),
                ("monitor", ctypes.c_bool),
                ("pollintervall_ms", ctypes.c_size_t)]

class MapiResultEntry(ctypes.Structure):
    _fields_ = [("name", ctypes.c_char_p),
                ("value", ctypes.POINTER(None))]

@ctypes.CFUNCTYPE(None, ctypes.c_int, ctypes.c_char_p, ctypes.c_char_p)
def log_callback(level: ctypes.c_int, component: ctypes.c_char_p, message: ctypes.c_char_p) -> None:
    """
    Implements a simple logger callback that can be fed to mapiSetLogCallback.
    """
    level_to_name = ["TRACE", "DEBUG", "INFO", "WARN", "ERROR", "CRITICAL"]
    print(f"[{component.decode('ascii')}] [{level_to_name[level]}] {message.decode('ascii')}")

def result_to_dict(lib, result: ctypes.POINTER(None)) -> "str | dict":
    """
    Takes a reference to the library and a pointer to a results object and recursively translates it into a python object.
    """
    value = ctypes.c_void_p()
    if (lib.mapiResultGetValue(result, ctypes.pointer(value))):
        return ctypes.cast(value, ctypes.c_char_p).value.decode('ascii')
    else:
        num = lib.mapiResultGetEntries(result, None, 0)
        buf = (MapiResultEntry*num)()
        lib.mapiResultGetEntries(result, buf, num)
        return {entry.name.decode('ascii'): result_to_dict(lib, entry.value) for entry in buf}

ALLOWED_PROVIDERS = ("git", "system", "energy", "gpu")

class Environment():
    def __init__(self, providers=ALLOWED_PROVIDERS, verbose=False):
        self.libmeasureapi = ctypes.cdll.LoadLibrary(path_to_libmeasureapi())

        ## Tell ctypes the datatypes of the respective functons
        ## NOTE: leaving this out has caused issues with downcasted addresses for the handle in the past.
        self.libmeasureapi.mapiStartMeasure.argtypes = [MapiConfig]
        self.libmeasureapi.mapiStartMeasure.restype = ctypes.c_void_p
        self.libmeasureapi.mapiStopMeasure.argtypes = [ctypes.c_void_p]
        self.libmeasureapi.mapiStopMeasure.restype = ctypes.c_void_p
        self.libmeasureapi.mapiSetLogCallback.argtypes = [ctypes.c_void_p]
        self.libmeasureapi.mapiResultGetValue.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_void_p)]
        self.libmeasureapi.mapiResultGetValue.restype = ctypes.c_bool
        self.libmeasureapi.mapiResultGetEntries.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t]
        self.libmeasureapi.mapiResultGetEntries.restype = ctypes.c_size_t
        self.libmeasureapi.mapiResultFree.argtypes = [ctypes.c_void_p]

        self.handle = None
        self.providers = self.allowed_providers(providers)
        self.measurements = []
        self.verbose = verbose

        if self.verbose:
            self.libmeasureapi.mapiSetLogCallback(log_callback)

    def allowed_providers(self, providers):
        ret = []
        for i in providers:
            if i not in ALLOWED_PROVIDERS:
                raise ValueError(f'Unallowed provider: "{i}".')
            ret.append(i.encode())
        ret.append(None)
        return ret
    
    def __enter__(self):
        self.start_measuring()
        return self

    def __exit__(self, *args):
        self.stop_measuring()

    def measure(self, func=None):
        if func:
            def wrapper(*arg):
                with self.measure():
                    return func(*arg)
            return wrapper

        return self

    def start_measuring(self):
        if self.handle:
            raise ValueError('Measuring was alraedy started...')

        providers_native = (ctypes.c_char_p*len(self.providers))(*self.providers)
        config = MapiConfig(providers_native, True, 100)
        self.handle = self.libmeasureapi.mapiStartMeasure(config)
    
    def stop_measuring(self):
        if not self.handle:
            raise ValueError('measuring was not yet started...')

        result = self.libmeasureapi.mapiStopMeasure(self.handle)
        obj = result_to_dict(self.libmeasureapi, result)
        self.libmeasureapi.mapiResultFree(result)
        self.measurements.append(obj)
        return obj
