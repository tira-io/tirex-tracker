import ctypes
import time
import json

class MeasureConfig(ctypes.Structure):
    _fields_ = [("provider", ctypes.POINTER(ctypes.c_char_p)),
                ("monitor", ctypes.c_bool),
                ("pollintervall_ms", ctypes.c_size_t)]

class MeasureResultEntry(ctypes.Structure):
    _fields_ = [("name", ctypes.c_char_p),
                ("value", ctypes.POINTER(None))]

@ctypes.CFUNCTYPE(None, ctypes.c_int, ctypes.c_char_p, ctypes.c_char_p)
def log_callback(level: ctypes.c_int, component: ctypes.c_char_p, message: ctypes.c_char_p) -> None:
    """
    Implements a simple logger callback that can be fed to msrSetLogCallback.
    """
    level_to_name = ["TRACE", "DEBUG", "INFO", "WARN", "ERROR", "CRITICAL"]
    print(f"[{component.decode('ascii')}] [{level_to_name[level]}] {message.decode('ascii')}")

def result_to_dict(lib, result: ctypes.c_void_p) -> "str | dict":
    """
    Takes a reference to the library and a pointer to a results object and recursively translates it into a python object.
    """
    value = ctypes.c_void_p()
    if (lib.mapiResultGetValue(result, ctypes.pointer(value))):
        return ctypes.cast(value, ctypes.c_char_p).value.decode('ascii')
    else:
        num = lib.measureResultEntriesGetAll(result, None, 0)
        buf = (MeasureResultEntry*num)()
        lib.measureResultEntriesGetAll(result, buf, num)
        return {entry.name.decode('ascii'): result_to_dict(lib, entry.value) for entry in buf}
            

if __name__ == "__main__":
    # Step 1: Load the shard object (YOUR PATH MAY BE DIFFERENT!)
    libmeasureapi = ctypes.cdll.LoadLibrary("build/src/libmeasureapi.so")

    ## Tell ctypes the datatypes of the respective functons
    ## NOTE: leaving this out has caused issues with downcasted addresses for the handle in the past.
    libmeasureapi.mapiStartMeasure.argtypes = [MapiConfig]
    libmeasureapi.mapiStartMeasure.restype = ctypes.c_void_p
    libmeasureapi.mapiStopMeasure.argtypes = [ctypes.c_void_p]
    libmeasureapi.mapiStopMeasure.restype = ctypes.c_void_p
    libmeasureapi.mapiSetLogCallback.argtypes = [ctypes.c_void_p]
    libmeasureapi.mapiResultGetValue.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_void_p)]
    libmeasureapi.mapiResultGetValue.restype = ctypes.c_bool
    libmeasureapi.mapiResultGetEntries.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t]
    libmeasureapi.mapiResultGetEntries.restype = ctypes.c_size_t
    libmeasureapi.mapiResultFree.argtypes = [ctypes.c_void_p]

    # (Optional): Register logger
    libmeasureapi.msrSetLogCallback(log_callback)

    # Step 2: Start measuring
    providers = [b"git", b"system", b"energy", b"gpu", None]
    providers_native = (ctypes.c_char_p*len(providers))(*providers)
    config = MeasureConfig(providers_native, True, 100)
    handle = libmeasureapi.measureStartMeasure(config)

    # Step 3: Run your code
    time.sleep(2)

    # Step 4: Stop measuring and fetch a handle to the result (needs to be freed by us later!)
    result = libmeasureapi.measureStopMeasure(handle)
    obj = result_to_dict(libmeasureapi, result)

    # Step 5: Cleanup, delete the result from memory
    libmeasureapi.measureResultFree(result)

    # Step 6..N: Do what you want
    print(json.dumps(obj, indent=2))
