import unittest
from ctypes import pointer

from tirex_tracker import (
    _LIBRARY,
    _NULL_MEASURE_CONFIGURATION,
    Aggregation,
    LogLevel,
    _MeasureConfiguration,
    _Result,
    set_log_callback,
)

set_log_callback(lambda level, component, message: print(f"[{level}][{component}] {message}"))


class TestErrorHandling(unittest.TestCase):
    def test_error_thrown(self):
        _LIBRARY.tirexSetAbortLevel(LogLevel.WARN)
        set_log_callback(lambda level, component, message: print(f"[{level}][{component}] {message}"))

        def should_raise():
            measures = [_MeasureConfiguration(-7, Aggregation.NO.value), _NULL_MEASURE_CONFIGURATION]

            configs = (_MeasureConfiguration * (len(measures)))(*measures)
            resultptr = pointer(_Result())
            _LIBRARY.tirexFetchInfo(configs, pointer(resultptr))
            print(resultptr.contents)

        self.assertRaises(RuntimeError, should_raise)
